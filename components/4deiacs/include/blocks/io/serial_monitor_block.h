/**
 * @file serial_monitor_block.h
 * @brief Bloco de Monitorizacao Serial Generico.
 */
#pragma once

#include <string>
#include <vector>
#include "i_function_block.h"
#include "cJSON.h"

namespace Cefet {

/**
 * @brief Serial Monitor Service Interface Function Block.
 *
 * Recolhe os dados de multiplos ponteiros float e imprime-os no 
 * barramento UART (terminal serial) do ESP32 a cada ciclo de execucao.
 * Essencial para depuracao de malhas sem a necessidade de clientes Modbus.
 */
class SerialMonitorBlock : public IFunctionBlock {
public:
    SerialMonitorBlock(const std::string& block_id, size_t num_in);
    ~SerialMonitorBlock() override;

    bool initialize() override;
    std::string getId() const override;

    bool connectDataInput(const std::string& port_name, void* data_pointer) override;
    void triggerEventInput(const std::string& event_name) override;

    static IFunctionBlock* create(const std::string& block_id, cJSON* config);

private:
    std::string m_id;
    size_t m_num_in;
    std::vector<float*> m_inputs;
};

} /* namespace Cefet */