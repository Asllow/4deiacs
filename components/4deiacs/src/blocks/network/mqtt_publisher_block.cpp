#include "mqtt_publisher_block.h"
#include "block_registry.h"
#include "esp_log.h"
#include <string>

namespace deiacs {

static const char* TAG = "MQTT_PUBLISHER_BLOCK";

MqttPublisherBlock::MqttPublisherBlock(const std::string& block_id, const std::string& broker_uri, const std::string& target_topic)
    : m_id(block_id), m_broker_uri(broker_uri), m_topic(target_topic), m_client(nullptr), m_is_connected(false), m_data_in(nullptr)
{
}

MqttPublisherBlock::~MqttPublisherBlock()
{
    if (m_client != nullptr) {
        esp_mqtt_client_stop(m_client);
        esp_mqtt_client_destroy(m_client);
    }
}

bool MqttPublisherBlock::initialize()
{
    esp_mqtt_client_config_t mqtt_cfg = {};
    mqtt_cfg.broker.address.uri = m_broker_uri.c_str();

    m_client = esp_mqtt_client_init(&mqtt_cfg);
    if (m_client == nullptr) {
        ESP_LOGE(TAG, "[%s] Failed to allocate memory for MQTT client.", m_id.c_str());
        return false;
    }

    esp_mqtt_client_register_event(m_client, MQTT_EVENT_ANY, &MqttPublisherBlock::mqttEventHandler, this);

    esp_err_t err = esp_mqtt_client_start(m_client);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "[%s] Failed to start MQTT state machine.", m_id.c_str());
        return false;
    }

    ESP_LOGI(TAG, "[%s] MQTT Block initialized. Target: %s", m_id.c_str(), m_broker_uri.c_str());
    return true;
}

std::string MqttPublisherBlock::getId() const
{
    return m_id;
}

bool MqttPublisherBlock::publish(const std::string& payload)
{
    if (m_client == nullptr || !m_is_connected) {
        ESP_LOGW(TAG, "[%s] Publish aborted: Client disconnected.", m_id.c_str());
        return false;
    }

    int msg_id = esp_mqtt_client_publish(m_client, m_topic.c_str(), payload.c_str(), 0, 1, 0);
    if (msg_id == -1) {
        ESP_LOGE(TAG, "[%s] Error enqueuing MQTT message.", m_id.c_str());
        return false;
    }

    return true;
}

bool MqttPublisherBlock::connectDataInput(const std::string& port_name, void* data_pointer)
{
    if (port_name == "IN_1" || port_name == "PAYLOAD") {
        m_data_in = static_cast<float*>(data_pointer);
        return true;
    }
    return false;
}

void MqttPublisherBlock::triggerEventInput(const std::string& event_name)
{
    if (event_name == "REQ" || event_name == "SEND") {
        
        float value = (m_data_in != nullptr) ? *m_data_in : 0.0f;
        std::string payload = std::to_string(value);
        
        publish(payload);
        
        emitEvent("CNF"); 
    }
}

void MqttPublisherBlock::mqttEventHandler(void* handler_args, esp_event_base_t base, int32_t event_id, void* event_data)
{
    auto* instance = static_cast<MqttPublisherBlock*>(handler_args);
    auto* event = static_cast<esp_mqtt_event_handle_t>(event_data);

    switch (event->event_id) {
        case MQTT_EVENT_CONNECTED:
            instance->m_is_connected = true;
            ESP_LOGI(TAG, "[%s] Connected to MQTT Broker.", instance->m_id.c_str());
            break;
        case MQTT_EVENT_DISCONNECTED:
            instance->m_is_connected = false;
            ESP_LOGW(TAG, "[%s] Disconnected from MQTT Broker.", instance->m_id.c_str());
            break;
        case MQTT_EVENT_ERROR:
            ESP_LOGE(TAG, "[%s] Internal MQTT protocol error.", instance->m_id.c_str());
            break;
        default:
            break;
    }
}

IFunctionBlock* MqttPublisherBlock::create(const std::string& block_id, cJSON* config)
{
    std::string broker = "mqtt://test.mosquitto.org";
    std::string topic = "4deiacs/default";

    if (config != nullptr) {
        cJSON* uri_item = cJSON_GetObjectItem(config, "broker_uri");
        if (cJSON_IsString(uri_item) && (uri_item->valuestring != nullptr)) {
            broker = uri_item->valuestring;
        }

        cJSON* topic_item = cJSON_GetObjectItem(config, "target_topic");
        if (cJSON_IsString(topic_item) && (topic_item->valuestring != nullptr)) {
            topic = topic_item->valuestring;
        }
    }

    return new MqttPublisherBlock(block_id, broker, topic);
}

static bool registered = []() {
    BlockRegistry::registerBlock("MqttPublisher", MqttPublisherBlock::create);
    return true;
}();

} // namespace deiacs