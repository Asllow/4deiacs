#pragma once

#include <string>
#include "i_function_block.h"
#include "lwip/sockets.h"
#include "cJSON.h"

namespace Cefet {

/**
 * @brief UDP Peer-to-Peer Publisher Service Interface Function Block (CSIFB).
 *
 * Implements fast, connectionless UDP datagram transmission.
 * Engineered for low-latency machine-to-machine communication 
 * within the same local network subnet.
 */
class UdpPublisherBlock : public IFunctionBlock {
public:
    /**
     * @brief Instantiates the UDP Publisher block.
     *
     * @param block_id Unique identifier for the block instance.
     * @param target_ip Destination IP address (e.g., "192.168.1.50" or "255.255.255.255").
     * @param target_port Destination UDP port.
     */
    UdpPublisherBlock(const std::string& block_id, const std::string& target_ip, uint16_t target_port);

    /**
     * @brief Destroys the UDP block and closes the operating system socket.
     */
    ~UdpPublisherBlock() override;

    /**
     * @brief Allocates the BSD socket and configures the destination address structure.
     *
     * @return true if the socket is successfully created.
     */
    bool initialize() override;

    /**
     * @brief Retrieves the block's unique identifier.
     *
     * @return std::string The configured block ID.
     */
    std::string getId() const override;

    /**
     * @brief Transmits a datagram directly to the configured IP/Port.
     *
     * @param payload Raw string payload to be transmitted.
     * @return true if the packet was handed over to the network interface.
     */
    bool publish(const std::string& payload);

    // =========================================================================
    // PORTAS IEC 61499
    // =========================================================================

    /**
     * @brief Recebe o ponteiro de outro bloco para a porta "PAYLOAD_IN".
     */
    bool connectDataInput(const std::string& port_name, void* data_pointer) override;

    /**
     * @brief Executa o disparo da mensagem ao receber o evento "SEND".
     */
    void triggerEventInput(const std::string& event_name) override;

    /**
     * @brief Factory method for dynamic instantiation via JSON manifest.
     *
     * @param block_id Unique identifier for the new instance.
     * @param config cJSON pointer containing block-specific parameters.
     * @return IFunctionBlock* Pointer to the newly allocated instance.
     */
    static IFunctionBlock* create(const std::string& block_id, cJSON* config);

private:
    std::string m_id;
    std::string m_target_ip;
    uint16_t m_port;
    int m_socket;
    struct sockaddr_in m_dest_addr;

    /** @brief Fio de Cobre virtual: aponta para a memoria de saida de outro bloco */
    float* m_payload_in; 
};

} // namespace Cefet