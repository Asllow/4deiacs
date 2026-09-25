/**
 * @file serial_monitor_block.h
 * @brief Generic Serial Monitor Block.
 */
#pragma once

#include <string>
#include <vector>
#include "i_function_block.h"
#include "cJSON.h"

namespace deiacs {

/**
 * @brief Serial Monitor Service Interface Function Block.
 *
 * Collects data from multiple float pointers and prints them to the 
 * ESP32 UART bus (serial terminal) on every execution cycle.
 * Essential for mesh debugging without the need for Modbus clients.
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

} /* namespace deiacs */