#pragma once

#include <string>
#include "i_function_block.h"
#include "mqtt_client.h"
#include "cJSON.h"

namespace Cefet {

/**
 * @brief Bloco de Serviço de Publicação MQTT (CSIFB).
 *
 * Encapsula o cliente MQTT nativo do ESP-IDF para atuar como um nó de publicação.
 * Roteia eventos e dados lidos da malha de controle local para um Broker MQTT externo.
 */
class MqttPublisherBlock : public IFunctionBlock {
public:
    /**
     * @brief Construtor do bloco de publicação MQTT.
     * * @param block_id Identificador único da instância do bloco.
     * @param broker_uri URI completa do broker MQTT (ex: "mqtt://broker.hivemq.com").
     * @param target_topic Tópico MQTT alvo onde as mensagens serão publicadas.
     */
    MqttPublisherBlock(const std::string& block_id, const std::string& broker_uri, const std::string& target_topic);

    /**
     * @brief Destrutor virtual. Encerra o cliente e liberta os recursos de rede.
     */
    ~MqttPublisherBlock() override;

    /**
     * @brief Inicializa o cliente MQTT e inicia a máquina de estados de conexão.
     * * @return true Se a alocação e o início do cliente foram bem-sucedidos.
     * @return false Se houve falha de memória ou configuração.
     */
    bool initialize() override;

    /**
     * @brief Recupera o identificador único do bloco.
     * * @return std::string O ID configurado.
     */
    std::string getId() const override;

    /**
     * @brief Envia uma string diretamente para o broker no tópico configurado.
     * * @param payload String contendo a carga útil dos dados.
     * @return true Se a mensagem foi enfileirada com sucesso.
     * @return false Se o cliente estiver desconectado.
     */
    bool publish(const std::string& payload);

    /**
     * @brief Vincula um ponteiro de dados externo à porta de entrada (Fio Azul).
     * * @param port_name Nome da porta de entrada (esperado: "IN_1").
     * @param data_pointer Ponteiro para a variável float de origem.
     * @return true Se a porta foi identificada e vinculada com sucesso.
     * @return false Se a porta for inválida.
     */
    bool connectDataInput(const std::string& port_name, void* data_pointer) override;

    /**
     * @brief Processa o disparo de um evento de entrada (Fio Vermelho).
     * * @param event_name Nome do evento recebido (esperado: "REQ").
     */
    void triggerEventInput(const std::string& event_name) override;

    /**
     * @brief Factory method para instanciação dinâmica via manifesto JSON.
     * * @param block_id Identificador único para a nova instância.
     * @param config Ponteiro cJSON contendo os parâmetros de configuração do bloco.
     * @return IFunctionBlock* Ponteiro para a instância alocada na memória.
     */
    static IFunctionBlock* create(const std::string& block_id, cJSON* config);

private:
    std::string m_id;
    std::string m_broker_uri;
    std::string m_topic;
    esp_mqtt_client_handle_t m_client;
    bool m_is_connected;
    float* m_data_in;

    /**
     * @brief Callback estático interno para tratamento de eventos do driver MQTT do ESP-IDF.
     */
    static void mqttEventHandler(void* handler_args, esp_event_base_t base, int32_t event_id, void* event_data);
};

} // namespace Cefet