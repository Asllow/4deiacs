#include "udp_publisher_block.h"
#include "block_registry.h"
#include "esp_log.h"
#include <cstring>

namespace Cefet {

static const char* TAG = "UDP_PUB_BLOCK";

UdpPublisherBlock::UdpPublisherBlock(const std::string& block_id, const std::string& target_ip, uint16_t target_port)
    : m_id(block_id), m_target_ip(target_ip), m_port(target_port), m_socket(-1), m_payload_in(nullptr)
{
    memset(&m_dest_addr, 0, sizeof(m_dest_addr));
    m_dest_addr.sin_family = AF_INET;
    m_dest_addr.sin_port = htons(m_port);
    m_dest_addr.sin_addr.s_addr = inet_addr(m_target_ip.c_str());
}

UdpPublisherBlock::~UdpPublisherBlock()
{
    if (m_socket != -1) {
        close(m_socket);
    }
}

bool UdpPublisherBlock::initialize()
{
    m_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
    if (m_socket < 0) {
        ESP_LOGE(TAG, "[%s] Failed to create UDP socket.", m_id.c_str());
        return false;
    }

    ESP_LOGI(TAG, "[%s] UDP Block initialized. Target: %s:%d", m_id.c_str(), m_target_ip.c_str(), m_port);
    return true;
}

std::string UdpPublisherBlock::getId() const
{
    return m_id;
}

bool UdpPublisherBlock::publish(const std::string& payload)
{
    if (m_socket < 0) return false;

    int err = sendto(m_socket, payload.c_str(), payload.length(), 0, 
                     (struct sockaddr *)&m_dest_addr, sizeof(m_dest_addr));

    if (err < 0) {
        ESP_LOGE(TAG, "[%s] Failed to send UDP datagram.", m_id.c_str());
        return false;
    }
    
    return true;
}

// =========================================================================
// IMPLEMENTACAO DAS PORTAS IEC 61499
// =========================================================================

bool UdpPublisherBlock::connectDataInput(const std::string& port_name, void* data_pointer)
{
    if (port_name == "PAYLOAD_IN") {
        // Recebe a "ponta do fio" do bloco anterior e pluga na nossa variavel
        m_payload_in = static_cast<float*>(data_pointer);
        return true;
    }
    return false;
}

void UdpPublisherBlock::triggerEventInput(const std::string& event_name)
{
    if (event_name == "SEND") {
        if (m_payload_in != nullptr) {
            // Acessa o espaco de memoria alheio, converte e dispara para a rede
            publish(std::to_string(*m_payload_in));
        } else {
            ESP_LOGE(TAG, "[%s] Tentativa de SEND, mas a porta PAYLOAD_IN esta desconectada!", m_id.c_str());
        }
    }
}

// =========================================================================

IFunctionBlock* UdpPublisherBlock::create(const std::string& block_id, cJSON* config)
{
    std::string ip = "255.255.255.255"; // Default: Broadcast
    uint16_t port = 5000;               // Default port

    if (config != nullptr) {
        cJSON* ip_item = cJSON_GetObjectItem(config, "target_ip");
        if (cJSON_IsString(ip_item) && (ip_item->valuestring != nullptr)) {
            ip = ip_item->valuestring;
        }

        cJSON* port_item = cJSON_GetObjectItem(config, "target_port");
        if (cJSON_IsNumber(port_item)) {
            port = static_cast<uint16_t>(port_item->valueint);
        }
    }

    return new UdpPublisherBlock(block_id, ip, port);
}

/**
 * @brief Static block registration.
 * Executes prior to app_main to register the factory method into the BlockRegistry.
 */
static bool registered = []() {
    BlockRegistry::registerBlock("UdpPublisher", UdpPublisherBlock::create);
    return true;
}();

} // namespace Cefet