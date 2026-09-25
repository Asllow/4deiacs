/**
 * @file modbus_tcp_server_block.h
 * @brief Dynamic and Generic Modbus TCP Server.
 */
#pragma once

#include <string>
#include <vector>
#include "i_function_block.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "cJSON.h"

namespace deiacs {

/**
 * @brief Modbus TCP Server Service Interface Function Block (CSIFB).
 *
 * Dynamically scalable implementation over BSD sockets. 
 * Supports configurable amount of Holding Registers in JSON.
 * Performs automatic float to 16-bit integer conversion 
 * using 2's complement, allowing SCADA traffic of negative numbers.
 */
class ModbusTcpServerBlock : public IFunctionBlock {
public:
    ModbusTcpServerBlock(const std::string& block_id, uint16_t port, uint16_t num_in, uint16_t num_out);
    ~ModbusTcpServerBlock() override;

    bool initialize() override;
    std::string getId() const override;

    void* getDataOutput(const std::string& port_name) override;
    bool connectDataInput(const std::string& port_name, void* data_pointer) override;
    void triggerEventInput(const std::string& event_name) override;

    static IFunctionBlock* create(const std::string& block_id, cJSON* config);

private:
    std::string m_id;
    uint16_t m_port;
    uint16_t m_num_in;
    uint16_t m_num_out;
    
    int m_listen_sock;
    TaskHandle_t m_server_task;
    
    /* Arrays dinamicos alocados no construtor. Substituem o codigo engessado. */
    std::vector<uint16_t> m_holding_registers;
    std::vector<float*> m_regs_in; 
    std::vector<float> m_regs_out;

    static void serverTask(void* arg);
};

} /* namespace deiacs */