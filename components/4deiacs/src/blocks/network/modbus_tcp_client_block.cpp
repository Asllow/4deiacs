/**
 * @file modbus_tcp_client_block.cpp
 * @brief Implementacao do bloco Modbus TCP Client.
 */
#include "blocks/network/modbus_tcp_client_block.h"
#include "block_registry.h"
#include "esp_log.h"
#include <lwip/sockets.h>
#include <cstring>

namespace Cefet {

static const char* TAG = "MODBUS_TCP_CLIENT_BLOCK";

ModbusTcpClientBlock::ModbusTcpClientBlock(const std::string& block_id, const std::string& target_ip, int port, int slave_id, int reg_addr)
    : m_id(block_id), m_target_ip(target_ip), m_port(port), m_slave_id(slave_id), m_reg_addr(reg_addr),
      m_sock(-1), m_initialized(false), m_transaction_id(0), m_data_in(nullptr)
{
}

ModbusTcpClientBlock::~ModbusTcpClientBlock()
{
    disconnect();
}

bool ModbusTcpClientBlock::initialize()
{
    if (m_initialized) return true;

    m_initialized = connectToServer();
    return m_initialized;
}

std::string ModbusTcpClientBlock::getId() const { return m_id; }

void* ModbusTcpClientBlock::getDataOutput(const std::string& port_name)
{
    return nullptr;
}

bool ModbusTcpClientBlock::connectDataInput(const std::string& port_name, void* data_pointer)
{
    if (port_name == "DATA_IN" && data_pointer != nullptr) {
        m_data_in = static_cast<float*>(data_pointer);
        return true;
    }
    return false;
}

void ModbusTcpClientBlock::triggerEventInput(const std::string& event_name)
{
    if (event_name == "REQ") {
        if (!m_initialized) {
            initialize();
        }

        if (m_initialized && m_data_in != nullptr) {
            uint16_t value_to_write = static_cast<uint16_t>(*m_data_in);
            
            if (sendWriteSingleRegister(value_to_write)) {
                emitEvent("CNF");
            } else {
                disconnect();
                m_initialized = false;
            }
        }
    }
}

bool ModbusTcpClientBlock::connectToServer()
{
    struct sockaddr_in dest_addr = {};
    dest_addr.sin_addr.s_addr = inet_addr(m_target_ip.c_str());
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(m_port);

    m_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
    if (m_sock < 0) {
        ESP_LOGE(TAG, "[%s] Falha ao criar socket TCP.", m_id.c_str());
        return false;
    }

    struct timeval timeout;
    timeout.tv_sec = 1;
    timeout.tv_usec = 0;
    setsockopt(m_sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
    setsockopt(m_sock, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));

    if (connect(m_sock, (struct sockaddr *)&dest_addr, sizeof(dest_addr)) != 0) {
        ESP_LOGW(TAG, "[%s] Falha ao conectar no servidor Modbus %s:%d.", m_id.c_str(), m_target_ip.c_str(), m_port);
        disconnect();
        return false;
    }

    ESP_LOGI(TAG, "[%s] Conectado ao servidor Modbus %s:%d", m_id.c_str(), m_target_ip.c_str(), m_port);
    return true;
}

void ModbusTcpClientBlock::disconnect()
{
    if (m_sock >= 0) {
        shutdown(m_sock, SHUT_RDWR);
        close(m_sock);
        m_sock = -1;
    }
}

bool ModbusTcpClientBlock::sendWriteSingleRegister(uint16_t value)
{
    if (m_sock < 0) return false;

    uint8_t frame[12];
    m_transaction_id++;

    frame[0] = (m_transaction_id >> 8) & 0xFF;
    frame[1] = m_transaction_id & 0xFF;
    frame[2] = 0x00; 
    frame[3] = 0x00;
    frame[4] = 0x00; 
    frame[5] = 0x06; 
    frame[6] = static_cast<uint8_t>(m_slave_id);
    frame[7] = 0x06; 
    frame[8] = (m_reg_addr >> 8) & 0xFF;
    frame[9] = m_reg_addr & 0xFF;
    frame[10] = (value >> 8) & 0xFF;
    frame[11] = value & 0xFF;

    int written = send(m_sock, frame, sizeof(frame), 0);
    if (written != sizeof(frame)) {
        return false;
    }

    uint8_t rx_buffer[12];
    recv(m_sock, rx_buffer, sizeof(rx_buffer), 0);

    return true;
}

IFunctionBlock* ModbusTcpClientBlock::create(const std::string& block_id, cJSON* config)
{
    std::string target_ip = "192.168.0.100";
    int port = 502;
    int slave_id = 1;
    int reg_addr = 0;

    if (config != nullptr) {
        cJSON* ip_item = cJSON_GetObjectItem(config, "target_ip");
        if (cJSON_IsString(ip_item)) target_ip = ip_item->valuestring;

        cJSON* port_item = cJSON_GetObjectItem(config, "port");
        if (cJSON_IsNumber(port_item)) port = port_item->valueint;

        cJSON* slave_item = cJSON_GetObjectItem(config, "slave_id");
        if (cJSON_IsNumber(slave_item)) slave_id = slave_item->valueint;

        cJSON* reg_item = cJSON_GetObjectItem(config, "reg_addr");
        if (cJSON_IsNumber(reg_item)) reg_addr = reg_item->valueint;
    }

    return new ModbusTcpClientBlock(block_id, target_ip, port, slave_id, reg_addr);
}

/**
 * @brief Registo estatico na Factory.
 */
static bool registered = []() {
    BlockRegistry::registerBlock("ModbusTcpClient", ModbusTcpClientBlock::create);
    return true;
}();

} // namespace Cefet