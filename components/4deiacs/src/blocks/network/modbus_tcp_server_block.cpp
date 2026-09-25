/**
 * @file modbus_tcp_server_block.cpp
 * @brief Implementacao do Servidor Modbus TCP Dinamico.
 */
#include "modbus_tcp_server_block.h"
#include "block_registry.h"
#include "esp_log.h"
#include "lwip/sockets.h"
#include <cstring>
#include <cmath>

namespace Cefet {

static const char* TAG = "MODBUS_GENERICO";

ModbusTcpServerBlock::ModbusTcpServerBlock(const std::string& block_id, uint16_t port, uint16_t num_in, uint16_t num_out)
    : m_id(block_id), m_port(port), m_num_in(num_in), m_num_out(num_out), m_listen_sock(-1), m_server_task(nullptr)
{
    /* Alocacao da tabela unificada: [0 ... num_in-1] = Entradas | [num_in ... fim] = Saidas */
    m_holding_registers.resize(m_num_in + m_num_out, 0);
    m_regs_in.resize(m_num_in, nullptr);
    m_regs_out.resize(m_num_out, 0.0f);
}

ModbusTcpServerBlock::~ModbusTcpServerBlock()
{
    if (m_server_task != nullptr) {
        vTaskDelete(m_server_task);
    }
    if (m_listen_sock != -1) {
        close(m_listen_sock);
    }
}

bool ModbusTcpServerBlock::initialize()
{
    m_listen_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
    if (m_listen_sock < 0) {
        return false;
    }

    int opt = 1;
    setsockopt(m_listen_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in dest_addr = {};
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    dest_addr.sin_port = htons(m_port);

    if (bind(m_listen_sock, (struct sockaddr *)&dest_addr, sizeof(dest_addr)) != 0) {
        close(m_listen_sock);
        return false;
    }

    if (listen(m_listen_sock, 1) != 0) {
        return false;
    }

    if (xTaskCreate(&ModbusTcpServerBlock::serverTask, m_id.c_str(), 4096, this, 5, &m_server_task) != pdPASS) {
        return false;
    }
    
    ESP_LOGI(TAG, "[%s] Servidor Modbus Iniciado: Porta %d | Entradas: %d | Saidas: %d", 
             m_id.c_str(), m_port, m_num_in, m_num_out);
    return true;
}

std::string ModbusTcpServerBlock::getId() const 
{ 
    return m_id; 
}

void* ModbusTcpServerBlock::getDataOutput(const std::string& port_name)
{
    /* Busca dinamica pelo nome gerado no JSON */
    if (port_name.find("REG_OUT_") == 0) {
        int idx = std::stoi(port_name.substr(8));
        if (idx >= 0 && idx < m_num_out) {
            return &m_regs_out[idx];
        }
    }
    return nullptr;
}

bool ModbusTcpServerBlock::connectDataInput(const std::string& port_name, void* data_pointer)
{
    /* Varredura dinamica das portas de entrada */
    if (port_name.find("REG_IN_") == 0) {
        int idx = std::stoi(port_name.substr(7));
        if (idx >= 0 && idx < m_num_in) {
            m_regs_in[idx] = static_cast<float*>(data_pointer);
            return true;
        }
    }
    return false;
}

void ModbusTcpServerBlock::triggerEventInput(const std::string& event_name)
{
    if (event_name == "UPDATE") {
        /* Transformacao de float para int16_t mantendo o Complemento de 2 binario */
        for (uint16_t i = 0; i < m_num_in; i++) {
            if (m_regs_in[i] != nullptr) {
                float raw_val = *(m_regs_in[i]);
                int16_t signed_val = static_cast<int16_t>(std::round(raw_val));
                m_holding_registers[i] = static_cast<uint16_t>(signed_val);
            }
        }
        
        /* Reconstrucao dos comandos recebidos da rede para o motor C++ */
        for (uint16_t i = 0; i < m_num_out; i++) {
            uint16_t raw_reg = m_holding_registers[m_num_in + i];
            int16_t signed_reg = static_cast<int16_t>(raw_reg);
            m_regs_out[i] = static_cast<float>(signed_reg);
        }
        
        emitEvent("CNF");
    }
}

void ModbusTcpServerBlock::serverTask(void* arg)
{
    auto* instance = static_cast<ModbusTcpServerBlock*>(arg);
    uint16_t total_regs = instance->m_num_in + instance->m_num_out;

    while (1) {
        struct sockaddr_in source_addr;
        socklen_t addr_len = sizeof(source_addr);
        int sock = accept(instance->m_listen_sock, (struct sockaddr *)&source_addr, &addr_len);
        
        if (sock < 0) {
            continue;
        }
        ESP_LOGI(TAG, "Mestre Modbus Conectado.");

        while (1) {
            uint8_t rx[256];
            int len = recv(sock, rx, sizeof(rx), 0);
            if (len <= 0) {
                break;
            }

            if (len >= 12 && rx[2] == 0 && rx[3] == 0) {
                uint8_t func = rx[7];
                uint16_t start_addr = (rx[8] << 8) | rx[9];
                uint16_t quant = (rx[10] << 8) | rx[11];

                if (func == 3 && quant > 0 && (start_addr + quant) <= total_regs) {
                    uint8_t tx[256];
                    memcpy(tx, rx, 7);
                    uint16_t out_len = 3 + (quant * 2);
                    tx[4] = out_len >> 8; 
                    tx[5] = out_len & 0xFF;
                    tx[7] = 3; 
                    tx[8] = quant * 2;
                    
                    for (int i = 0; i < quant; i++) {
                        uint16_t val = instance->m_holding_registers[start_addr + i];
                        tx[9 + i*2] = val >> 8;
                        tx[10 + i*2] = val & 0xFF;
                    }
                    send(sock, tx, 9 + (quant * 2), 0);
                } 
                else if (func == 16 && quant > 0 && (start_addr + quant) <= total_regs) {
                    for (int i = 0; i < quant; i++) {
                        instance->m_holding_registers[start_addr + i] = (rx[13 + i*2] << 8) | rx[14 + i*2];
                    }
                    uint8_t tx[12];
                    memcpy(tx, rx, 12);
                    tx[4] = 0; 
                    tx[5] = 6;
                    send(sock, tx, 12, 0);
                }
            }
        }
        close(sock);
        ESP_LOGI(TAG, "Mestre Modbus Desconectado.");
    }
}

IFunctionBlock* ModbusTcpServerBlock::create(const std::string& block_id, cJSON* config) 
{
    uint16_t port = 502; 
    uint16_t in_regs = 4;
    uint16_t out_regs = 4;

    if (config != nullptr) {
        cJSON* p = cJSON_GetObjectItem(config, "port");
        if (cJSON_IsNumber(p)) {
            port = static_cast<uint16_t>(p->valueint);
        }

        cJSON* in = cJSON_GetObjectItem(config, "regs_in");
        if (cJSON_IsNumber(in)) {
            in_regs = static_cast<uint16_t>(in->valueint);
        }

        cJSON* out = cJSON_GetObjectItem(config, "regs_out");
        if (cJSON_IsNumber(out)) {
            out_regs = static_cast<uint16_t>(out->valueint);
        }
    }
    
    return new ModbusTcpServerBlock(block_id, port, in_regs, out_regs);
}

static bool registered = []() {
    BlockRegistry::registerBlock("ModbusTcpServer", ModbusTcpServerBlock::create);
    return true;
}();

} /* namespace Cefet */