#include "block_registry.h"
#include "esp_log.h"

namespace Cefet {

static const char* TAG = "BLOCK_REGISTRY";

std::unordered_map<std::string, BlockFactoryFunc>& BlockRegistry::getRegistry() {
    static std::unordered_map<std::string, BlockFactoryFunc> registry;
    return registry;
}

std::vector<IFunctionBlock*>& BlockRegistry::getInstances() {
    static std::vector<IFunctionBlock*> instances;
    return instances;
}

void BlockRegistry::registerBlock(const std::string& block_type, BlockFactoryFunc factory) {
    auto& registry = getRegistry();
    if (registry.find(block_type) == registry.end()) {
        registry[block_type] = factory;
        ESP_LOGD(TAG, "Bloco [%s] registrado na Fabrica.", block_type.c_str());
    } else {
        ESP_LOGW(TAG, "Tentativa de registrar o bloco [%s] em duplicidade.", block_type.c_str());
    }
}

IFunctionBlock* BlockRegistry::createBlock(const std::string& block_type, const std::string& block_id, cJSON* config) {
    auto& registry = getRegistry();
    auto it = registry.find(block_type);
    
    if (it != registry.end()) {
        IFunctionBlock* block = it->second(block_id, config);
        if (block != nullptr) {
            getInstances().push_back(block);
        }
        return block;
    }
    
    ESP_LOGE(TAG, "Tipo de bloco desconhecido ou nao registrado: [%s]", block_type.c_str());
    return nullptr;
}

void BlockRegistry::clearAll() {
    auto& instances = getInstances();
    
    for (IFunctionBlock* block : instances) {
        delete block;
    }
    
    instances.clear();
    ESP_LOGI(TAG, "Todas as instancias de blocos foram destruidas e desalocadas da RAM.");
}

} // namespace Cefet