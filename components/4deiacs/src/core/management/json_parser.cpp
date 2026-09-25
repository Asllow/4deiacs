#include "json_parser.h"
#include "block_registry.h"
#include "connection_manager.h"
#include "cJSON.h"
#include "esp_log.h"
#include <vector>

namespace deiacs {

static const char* TAG = "JSON_PARSER";

esp_err_t JsonParser::parseManifest(const char* json_payload, std::vector<IFunctionBlock*>& out_instances)
{
    if (json_payload == nullptr) {
        ESP_LOGE(TAG, "Null payload provided to parser.");
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "Starting JSON Manifest Parsing...");
    cJSON* root = cJSON_Parse(json_payload);
    if (root == nullptr) {
        ESP_LOGE(TAG, "JSON conversion failed. Invalid syntax.");
        return ESP_FAIL;
    }

    out_instances.clear();

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

                ESP_LOGD(TAG, "Instantiating block: [%s] of type <%s>", block_id.c_str(), block_type.c_str());

                IFunctionBlock* new_block = BlockRegistry::createBlock(block_type, block_id, config_obj);
                
                if (new_block != nullptr) {
                    if (new_block->initialize()) {
                        out_instances.push_back(new_block);
                    } else {
                        ESP_LOGE(TAG, "Initialization failure in block [%s]. Aborting instance.", block_id.c_str());
                        delete new_block;
                    }
                }
            }
        }
    } else {
        ESP_LOGW(TAG, "Manifest does not contain a valid 'blocks' array.");
    }
    cJSON* conns_array = cJSON_GetObjectItem(root, "connections");
    bool wiring_success = false;
    
    if (cJSON_IsArray(conns_array)) {
        wiring_success = ConnectionManager::wireConnections(conns_array, out_instances);
    } else {
        ESP_LOGW(TAG, "No connections found in manifest.");
    }
    cJSON_Delete(root);

    if (!wiring_success && cJSON_IsArray(conns_array)) {
        ESP_LOGE(TAG, "Critical failure during routing. Mesh may be inconsistent.");
        for (auto* b : out_instances) { delete b; }
        out_instances.clear();
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "Parsing and Construction completed successfully (%d active blocks).", out_instances.size());
    return ESP_OK;
}

} // namespace deiacs