#pragma once

#include <string>
#include "i_function_block.h"
#include "mqtt_client.h"
#include "cJSON.h"

namespace deiacs {

/**
 * @brief MQTT Publication Service Block (CSIFB).
 *
 * Encapsulates the native ESP-IDF MQTT client to act as a publishing node.
 * Routes events and data read from the local control mesh to an external MQTT Broker.
 */
class MqttPublisherBlock : public IFunctionBlock {
public:
    /**
     * @brief MQTT publication block constructor.
     * * @param block_id Unique identifier of the block instance.
     * @param broker_uri Complete MQTT broker URI (e.g. "mqtt://broker.hivemq.com").
     * @param target_topic Target MQTT topic where messages will be published.
     */
    MqttPublisherBlock(const std::string& block_id, const std::string& broker_uri, const std::string& target_topic);

    /**
     * @brief Virtual destructor. Terminates the client and frees network resources.
     */
    ~MqttPublisherBlock() override;

    /**
     * @brief Initializes the MQTT client and starts the connection state machine.
     * * @return true If the allocation and start of the client were successful.
     * @return false If there was a memory or configuration failure.
     */
    bool initialize() override;

    /**
     * @brief Retrieves the unique identifier of the block.
     * * @return std::string The configured ID.
     */
    std::string getId() const override;

    /**
     * @brief Sends a string directly to the broker on the configured topic.
     * * @param payload String containing the data payload.
     * @return true If the message was successfully queued.
     * @return false If the client is disconnected.
     */
    bool publish(const std::string& payload);

    /**
     * @brief Binds an external data pointer to the input port (Blue Wire).
     * * @param port_name Input port name (expected: "IN_1").
     * @param data_pointer Pointer to the source float variable.
     * @return true If the port was identified and successfully bound.
     * @return false If the port is invalid.
     */
    bool connectDataInput(const std::string& port_name, void* data_pointer) override;

    /**
     * @brief Processes the trigger of an input event (Red Wire).
     * * @param event_name Received event name (expected: "REQ").
     */
    void triggerEventInput(const std::string& event_name) override;

    /**
     * @brief Factory method for dynamic instantiation via JSON manifest.
     * * @param block_id Unique identifier for the new instance.
     * @param config cJSON pointer containing block configuration parameters.
     * @return IFunctionBlock* Pointer to the instance allocated in memory.
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
     * @brief Internal static callback for handling ESP-IDF MQTT driver events.
     */
    static void mqttEventHandler(void* handler_args, esp_event_base_t base, int32_t event_id, void* event_data);
};

} // namespace deiacs