/**
 * @file modbus_tcp_client_block.h
 * @brief Definition of the Modbus TCP Client block for data transmission.
 */
#pragma once

#include "i_function_block.h"
#include "cJSON.h"
#include <string>
#include <cstdint>

namespace deiacs {

/**
 * @class ModbusTcpClientBlock
 * @brief Network block acting as a Modbus TCP Client (Master) to write registers on a remote Server.
 */
class ModbusTcpClientBlock : public IFunctionBlock {
public:
    ModbusTcpClientBlock(const std::string& block_id, const std::string& target_ip, int port, int slave_id, int reg_addr);
    virtual ~ModbusTcpClientBlock();

    bool initialize();
    std::string getId() const override;

    void* getDataOutput(const std::string& port_name) override;
    bool connectDataInput(const std::string& port_name, void* data_pointer) override;
    void triggerEventInput(const std::string& event_name) override;

    static IFunctionBlock* create(const std::string& block_id, cJSON* config);

private:
    std::string m_id;
    std::string m_target_ip;
    int m_port;
    int m_slave_id;
    int m_reg_addr;

    int m_sock;
    bool m_initialized;
    uint16_t m_transaction_id;
    
    float* m_data_in;

    bool connectToServer();
    void disconnect();
    bool sendWriteSingleRegister(uint16_t value);
};

} // namespace deiacs