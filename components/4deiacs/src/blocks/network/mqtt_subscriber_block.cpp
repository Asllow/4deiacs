/**
 * @file mqtt_subscriber_block.cpp
 * @brief Implementacao do bloco assinante MQTT.
 */
#include "blocks/network/mqtt_subscriber_block.h"
#include "block_registry.h"
#include "esp_log.h"
#include <cstdlib>
#include <cstring>

namespace Cefet {

static const char* TAG = "MQTT_SUBSCRIBER_BLOCK";

MqttSubscriberBlock::MqttSubscriberBlock(const std::string& block_id, const std::string& broker_url, const std::string& topic)
    : m_id(block_id), m_broker_url(broker_url), m_topic(topic), m_client(nullptr), m_initialized(false), m_data_out(0.0f)
{
}

MqttSubscriberBlock::~MqttSubscriberBlock()
{
    if (m_client != nullptr) {
        esp_mqtt_client_stop(m_client);
        esp_mqtt_client_destroy(m_client);
    }
}

bool MqttSubscriberBlock::initialize()
{
    if (m_initialized) return true;

    esp_mqtt_client_config_t mqtt_cfg = {};
    mqtt_cfg.broker.address.uri = m_broker_url.c_str();

    m_client = esp_mqtt_client_init(&mqtt_cfg);
    if (m_client == nullptr) {
        ESP_LOGE(TAG, "[%s] Falha ao inicializar cliente MQTT.", m_id.c_str());
        return false;
    }

    esp_mqtt_client_register_event(m_client, MQTT_EVENT_ANY, mqttEventHandler, this);
    esp_mqtt_client_start(m_client);

    m_initialized = true;
    ESP_LOGI(TAG, "[%s] Bloco MQTT Subscriber inicializado. Topico: %s", m_id.c_str(), m_topic.c_str());
    return true;
}

std::string MqttSubscriberBlock::getId() const { return m_id; }

void* MqttSubscriberBlock::getDataOutput(const std::string& port_name)
{
    if (port_name == "DATA_OUT") return &m_data_out;
    return nullptr;
}

void MqttSubscriberBlock::triggerEventInput(const std::string& event_name)
{
    if (!m_initialized && event_name == "INIT") {
        initialize();
    }
}

void MqttSubscriberBlock::mqttEventHandler(void* handler_args, esp_event_base_t base, int32_t event_id, void* event_data)
{
    auto* block = static_cast<MqttSubscriberBlock*>(handler_args);
    esp_mqtt_event_handle_t event = static_cast<esp_mqtt_event_handle_t>(event_data);

    switch (static_cast<esp_mqtt_event_id_t>(event_id)) {
        case MQTT_EVENT_CONNECTED:
            esp_mqtt_client_subscribe(block->m_client, block->m_topic.c_str(), 0);
            ESP_LOGI(TAG, "[%s] Conectado e inscrito: %s", block->m_id.c_str(), block->m_topic.c_str());
            break;
            
        case MQTT_EVENT_DATA:
            if (event->topic_len == block->m_topic.length() && strncmp(event->topic, block->m_topic.c_str(), event->topic_len) == 0) {
                char payload[64] = {0};
                int len = (event->data_len < 63) ? event->data_len : 63;
                strncpy(payload, event->data, len);
                
                block->m_data_out = std::strtof(payload, nullptr);
                block->emitEvent("IND");
            }
            break;
            
        default:
            break;
    }
}

IFunctionBlock* MqttSubscriberBlock::create(const std::string& block_id, cJSON* config)
{
    std::string broker_url = "mqtt://192.168.0.23"; 
    std::string topic = "lss/data";

    if (config != nullptr) {
        cJSON* url_item = cJSON_GetObjectItem(config, "broker_url");
        if (cJSON_IsString(url_item)) broker_url = url_item->valuestring;

        cJSON* topic_item = cJSON_GetObjectItem(config, "topic");
        if (cJSON_IsString(topic_item)) topic = topic_item->valuestring;
    }

    return new MqttSubscriberBlock(block_id, broker_url, topic);
}

/**
 * @brief Registo estatico na Factory.
 */
static bool registered = []() {
    BlockRegistry::registerBlock("MqttSubscriber", MqttSubscriberBlock::create);
    return true;
}();

} // namespace Cefet