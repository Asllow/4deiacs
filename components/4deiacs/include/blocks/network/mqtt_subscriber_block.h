/**
 * @file mqtt_subscriber_block.h
 * @brief Definition of the MQTT subscriber block for event architecture.
 */
#pragma once

#include "cJSON.h"
#include "i_function_block.h"
#include "mqtt_client.h"
#include <string>

namespace deiacs {

/**
 * @class MqttSubscriberBlock
 * @brief Network block that listens to an MQTT topic and emits IND event upon
 * receiving data.
 */
class MqttSubscriberBlock : public IFunctionBlock {
public:
    MqttSubscriberBlock(const std::string &block_id, const std::string &broker_url,
                        const std::string &topic);
    virtual ~MqttSubscriberBlock();

    bool initialize() override;
    std::string getId() const override;

    void *getDataOutput(const std::string &port_name) override;
    void triggerEventInput(const std::string &event_name) override;

    static IFunctionBlock *create(const std::string &block_id, cJSON *config);

private:
    std::string m_id;
    std::string m_broker_url;
    std::string m_topic;

    esp_mqtt_client_handle_t m_client;
    bool m_initialized;
    float m_data_out;

    static void mqttEventHandler(void *handler_args, esp_event_base_t base,
                                int32_t event_id, void *event_data);
};

} // namespace deiacs