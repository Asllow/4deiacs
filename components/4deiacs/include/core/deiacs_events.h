#pragma once

#include "esp_event.h"

namespace deiacs {

/**
 * @brief Declaration of the Event Family for the 4deiacs engine.
 * * Native ESP-IDF macro. Defines a base string so that the event bus
 * differentiates our system events from Wi-Fi or Bluetooth events.
 */
ESP_EVENT_DECLARE_BASE(DEIACS_CORE_EVENTS);

/**
 * @brief Enumeration of internal system event IDs.
 * * These IDs replace the traditional visual "event lines" of the 
 * IEC 61499 standard. They act as triggers that wake up 
 * the corresponding function blocks, eliminating the need for polling.
 */
enum EventIds {
    EV_SYSTEM_BOOT = 0,         /*!< System initialized and ready to operate. */
    EV_SENSOR_DATA_READY,       /*!< New hardware data converted by ADC/GPIO. */
    EV_NETWORK_CONNECTED,
    EV_CONTROL_CALC_DONE,       /*!< Algorithm (e.g. PID) finished its mathematical calculation. */
    EV_NETWORK_RX,              /*!< Data packet received over network (MQTT/Modbus/UDP). */
    EV_CONFIG_UPDATED           /*!< New JSON file received, requests hot re-deployment. */
};

} // namespace deiacs