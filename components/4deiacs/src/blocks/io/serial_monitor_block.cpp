/**
 * @file serial_monitor_block.cpp
 * @brief Implementacao da Sonda Serial.
 */
#include "serial_monitor_block.h"
#include "block_registry.h"
#include "esp_log.h"

namespace Cefet {

static const char* TAG = "SERIAL_MONITOR";

SerialMonitorBlock::SerialMonitorBlock(const std::string& block_id, size_t num_in)
    : m_id(block_id), m_num_in(num_in)
{
    m_inputs.resize(m_num_in, nullptr);
}

SerialMonitorBlock::~SerialMonitorBlock() = default;

bool SerialMonitorBlock::initialize()
{
    ESP_LOGI(TAG, "[%s] Monitor Serial inicializado com %zu portas de entrada.", m_id.c_str(), m_num_in);
    return true;
}

std::string SerialMonitorBlock::getId() const
{
    return m_id;
}

bool SerialMonitorBlock::connectDataInput(const std::string& port_name, void* data_pointer)
{
    if (port_name.find("IN_") == 0) {
        size_t idx = std::stoi(port_name.substr(3));
        if (idx < m_num_in) {
            m_inputs[idx] = static_cast<float*>(data_pointer);
            return true;
        }
    }
    return false;
}

void SerialMonitorBlock::triggerEventInput(const std::string& event_name)
{
    if (event_name == "REQ") {
        std::string output = "[" + m_id + "] Leitura: ";
        
        /* Concatena os valores de todas as portas ligadas */
        for (size_t i = 0; i < m_num_in; ++i) {
            if (m_inputs[i] != nullptr) {
                output += "IN_" + std::to_string(i) + " = " + std::to_string(*(m_inputs[i])) + " | ";
            }
        }
        
        ESP_LOGI(TAG, "%s", output.c_str());
        emitEvent("CNF");
    }
}

IFunctionBlock* SerialMonitorBlock::create(const std::string& block_id, cJSON* config)
{
    size_t num_in = 1;
    if (config != nullptr) {
        cJSON* in_item = cJSON_GetObjectItem(config, "num_in");
        if (cJSON_IsNumber(in_item)) {
            num_in = static_cast<size_t>(in_item->valueint);
        }
    }
    return new SerialMonitorBlock(block_id, num_in);
}

static bool registered = []() {
    BlockRegistry::registerBlock("SerialMonitor", SerialMonitorBlock::create);
    return true;
}();

} /* namespace Cefet */