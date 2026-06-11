#include "json_parser.h"
#include "block_registry.h"
#include "connection_manager.h"
#include "cJSON.h"
#include "esp_log.h"
#include <vector>

namespace Cefet {

static const char* TAG = "JSON_PARSER";

esp_err_t JsonParser::parseManifest(const char* json_payload)
{
    if (json_payload == nullptr) {
        ESP_LOGE(TAG, "Payload nulo fornecido ao parser.");
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "A iniciar Parsing do Manifesto JSON...");
    cJSON* root = cJSON_Parse(json_payload);
    if (root == nullptr) {
        ESP_LOGE(TAG, "Falha na conversao do JSON. Sintaxe invalida.");
        return ESP_FAIL;
    }

    std::vector<IFunctionBlock*> instantiated_blocks;

    cJSON* blocks_array = cJSON_GetObjectItem(root, "blocks");
    if (cJSON_IsArray(blocks_array)) {
        cJSON* block_item = nullptr;
        cJSON_ArrayForEach(block_item, blocks_array) {
            cJSON* id_obj = cJSON_GetObjectItem(block_item, "id");
            cJSON* type_obj = cJSON_GetObjectItem(block_item, "type");
            cJSON* config_obj = cJSON_GetObjectItem(block_item, "config");

            if (cJSON_IsString(id_obj) && cJSON_IsString(type_obj)) {
                std::string block_id = id_obj->valuestring;
                std::string block_type = type_obj->valuestring;

                ESP_LOGD(TAG, "A instanciar bloco: [%s] do tipo <%s>", block_id.c_str(), block_type.c_str());

                IFunctionBlock* new_block = BlockRegistry::createBlock(block_type, block_id, config_obj);
                
                if (new_block != nullptr) {
                    if (new_block->initialize()) {
                        instantiated_blocks.push_back(new_block);
                    } else {
                        ESP_LOGE(TAG, "Falha de inicializacao no bloco [%s]. A abortar instancia.", block_id.c_str());
                        delete new_block;
                    }
                }
            }
        }
    } else {
        ESP_LOGW(TAG, "O manifesto nao contem um array 'blocks' valido.");
    }
    cJSON* conns_array = cJSON_GetObjectItem(root, "connections");
    bool wiring_success = false;
    
    if (cJSON_IsArray(conns_array)) {
        wiring_success = ConnectionManager::wireConnections(conns_array, instantiated_blocks);
    } else {
        ESP_LOGW(TAG, "Nenhuma conexao encontrada no manifesto.");
    }
    cJSON_Delete(root);

    if (!wiring_success && cJSON_IsArray(conns_array)) {
        ESP_LOGE(TAG, "Falha critica durante o roteamento. A malha pode estar inconsistente.");
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "Parsing e Construcao concluidos com sucesso (%d blocos ativos).", instantiated_blocks.size());
    return ESP_OK;
}

} // namespace Cefet