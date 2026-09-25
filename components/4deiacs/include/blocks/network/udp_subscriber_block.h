/**
 * @file udp_subscriber_block.h
 * @brief Definicao do bloco assinante de pacotes UDP.
 */
#pragma once

#include "i_function_block.h"
#include "cJSON.h"
#include <string>
#include <thread>
#include <atomic>

namespace Cefet {

/**
 * @class UdpSubscriberBlock
 * @brief Bloco de rede que escuta datagramas UDP assincronamente e emite evento IND.
 */
class UdpSubscriberBlock : public IFunctionBlock {
public:
    UdpSubscriberBlock(const std::string& block_id, int port);
    virtual ~UdpSubscriberBlock();

    bool initialize();
    std::string getId() const override;

    void* getDataOutput(const std::string& port_name) override;
    void triggerEventInput(const std::string& event_name) override;

    static IFunctionBlock* create(const std::string& block_id, cJSON* config);

private:
    std::string m_id;
    int m_port;
    int m_sock;
    bool m_initialized;
    std::atomic<bool> m_is_running;
    std::thread m_listener_thread;

    float m_data_out;

    void listenerTask();
};

} // namespace Cefet