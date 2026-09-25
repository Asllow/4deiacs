#include "connection_manager.h"
#include "cJSON.h"
#include "esp_log.h"

namespace deiacs {

static const char* TAG = "CONNECTION_MANAGER";

static IFunctionBlock* findBlock(const std::vector<IFunctionBlock*>& blocks, const std::string& id) {
    for (auto* b : blocks) {
        if (b->getId() == id) {
            return b;
        }
    }
    return nullptr;
}

bool ConnectionManager::wireConnections(cJSON* conns_array, const std::vector<IFunctionBlock*>& blocks) {

    if (conns_array == nullptr || !cJSON_IsArray(conns_array)) {
        ESP_LOGW(TAG, "Manifest does not contain a 'connections' array. Routing aborted.");
        return false;
    }

    ESP_LOGI(TAG, "Starting IEC 61499 Routing (Wiring)...");

    cJSON* conn = nullptr;
    cJSON_ArrayForEach(conn, conns_array) {
        cJSON* src_node = cJSON_GetObjectItem(conn, "source");
        cJSON* tgt_node = cJSON_GetObjectItem(conn, "target");

        if (!cJSON_IsString(src_node) || !cJSON_IsString(tgt_node)) {
            continue;
        }

        std::string source_full = src_node->valuestring;
        std::string target_full = tgt_node->valuestring;

        size_t src_dot = source_full.find('.');
        size_t tgt_dot = target_full.find('.');

        if (src_dot == std::string::npos || tgt_dot == std::string::npos) {
            ESP_LOGE(TAG, "Connection syntax error (Missing dot): %s -> %s", source_full.c_str(), target_full.c_str());
            continue;
        }

        std::string src_id = source_full.substr(0, src_dot);
        std::string src_port = source_full.substr(src_dot + 1);

        std::string tgt_id = target_full.substr(0, tgt_dot);
        std::string tgt_port = target_full.substr(tgt_dot + 1);

        IFunctionBlock* src_block = findBlock(blocks, src_id);
        IFunctionBlock* tgt_block = findBlock(blocks, tgt_id);

        if (src_block == nullptr || tgt_block == nullptr) {
            ESP_LOGE(TAG, "Block missing on board. Failed to plug: %s -> %s", source_full.c_str(), target_full.c_str());
            continue;
        }

        void* data_ptr = src_block->getDataOutput(src_port);
        if (data_ptr != nullptr) {
            if (tgt_block->connectDataInput(tgt_port, data_ptr)) {
                ESP_LOGI(TAG, "DATA wire connected: [%s].%s ---> [%s].%s", src_id.c_str(), src_port.c_str(), tgt_id.c_str(), tgt_port.c_str());
            } else {
                ESP_LOGE(TAG, "Data input port %s rejected the connection.", target_full.c_str());
            }
        } else {
            src_block->connectEventOutput(src_port, tgt_block, tgt_port);
            ESP_LOGI(TAG, "EVENT wire connected: [%s].%s ---> [%s].%s", src_id.c_str(), src_port.c_str(), tgt_id.c_str(), tgt_port.c_str());
        }
    }

    return true;
}

void ConnectionManager::clearAll() {
    ESP_LOGI(TAG, "Routing undone (Wires destroyed natively in cascade by blocks).");
}

} // namespace deiacs