/**
 * @file udp_subscriber_block.cpp
 * @brief Implementacao do bloco assinante UDP.
 */
#include "blocks/network/udp_subscriber_block.h"
#include "block_registry.h"
#include "esp_log.h"
#include <lwip/sockets.h>
#include <cstdlib>

namespace Cefet {

static const char* TAG = "UDP_SUBSCRIBER_BLOCK";

UdpSubscriberBlock::UdpSubscriberBlock(const std::string& block_id, int port)
    : m_id(block_id), m_port(port), m_sock(-1), m_initialized(false), m_is_running(false), m_data_out(0.0f)
{
}

UdpSubscriberBlock::~UdpSubscriberBlock()
{
    m_is_running = false;
    if (m_sock >= 0) {
        shutdown(m_sock, SHUT_RDWR);
        close(m_sock);
    }
    if (m_listener_thread.joinable()) {
        m_listener_thread.join();
    }
}

bool UdpSubscriberBlock::initialize()
{
    if (m_initialized) return true;

    struct sockaddr_in dest_addr = {};
    dest_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(m_port);

    m_sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
    if (m_sock < 0) {
        ESP_LOGE(TAG, "[%s] Falha ao criar socket UDP.", m_id.c_str());
        return false;
    }

    if (bind(m_sock, (struct sockaddr *)&dest_addr, sizeof(dest_addr)) < 0) {
        ESP_LOGE(TAG, "[%s] Falha ao fazer bind na porta %d.", m_id.c_str(), m_port);
        close(m_sock);
        m_sock = -1;
        return false;
    }

    m_is_running = true;
    m_listener_thread = std::thread(&UdpSubscriberBlock::listenerTask, this);

    m_initialized = true;
    ESP_LOGI(TAG, "[%s] Bloco UDP inicializado, escutando porta %d.", m_id.c_str(), m_port);
    return true;
}

std::string UdpSubscriberBlock::getId() const { return m_id; }

void* UdpSubscriberBlock::getDataOutput(const std::string& port_name)
{
    if (port_name == "DATA_OUT") return &m_data_out;
    return nullptr;
}

void UdpSubscriberBlock::triggerEventInput(const std::string& event_name)
{
    // Subscriber gera eventos (IND), normalmente nao os recebe para processamento.
    if (!m_initialized && event_name == "INIT") {
        initialize();
    }
}

void UdpSubscriberBlock::listenerTask()
{
    char rx_buffer[128];
    while (m_is_running) {
        struct sockaddr_storage source_addr;
        socklen_t socklen = sizeof(source_addr);
        
        int len = recvfrom(m_sock, rx_buffer, sizeof(rx_buffer) - 1, 0, (struct sockaddr *)&source_addr, &socklen);
        
        if (len > 0) {
            rx_buffer[len] = '\0';
            // Atualiza a variavel apontada pelo motor e dispara o evento assincronamente
            m_data_out = std::strtof(rx_buffer, nullptr);
            emitEvent("IND");
        } else if (len < 0 && m_is_running) {
            // Em caso de falha silenciosa de rede, previne loop infinito de alto consumo
            vTaskDelay(pdMS_TO_TICKS(50)); 
        }
    }
}

IFunctionBlock* UdpSubscriberBlock::create(const std::string& block_id, cJSON* config)
{
    int port = 5000;
    if (config != nullptr) {
        cJSON* port_item = cJSON_GetObjectItem(config, "port");
        if (cJSON_IsNumber(port_item)) {
            port = port_item->valueint;
        }
    }
    return new UdpSubscriberBlock(block_id, port);
}

/**
 * @brief Registo estatico na Factory.
 */
static bool registered = []() {
    BlockRegistry::registerBlock("UdpSubscriber", UdpSubscriberBlock::create);
    return true;
}();

} // namespace Cefet