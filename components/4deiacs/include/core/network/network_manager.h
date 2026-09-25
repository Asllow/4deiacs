#pragma once

#include "esp_err.h"
#include "esp_event.h"

namespace deiacs {

/**
 * @brief Node Connectivity Stack Manager.
 *
 * Encapsulates the initialization of NVS memory, network interface (esp_netif),
 * native ESP32 Wi-Fi driver, and mDNS identification. Operates asynchronously
 * and publishes the EV_NETWORK_CONNECTED event on the central bus
 * when an IP is obtained.
 */
class NetworkManager {
public:
    /**
     * @brief Initializes Wi-Fi hardware, configures mDNS, and starts the connection attempt.
     * Uses credentials defined via menuconfig and hostname stored in NVS.
     *
     * @return esp_err_t ESP_OK if the connection process successfully started.
     */
    static esp_err_t connect();

private:
    /**
     * @brief Internal callback for handling native Wi-Fi and IP driver events.
     *
     * @param arg Context arguments.
     * @param event_base Event family (WIFI_EVENT or IP_EVENT).
     * @param event_id Specific event identifier.
     * @param event_data Associated data (e.g., received IP).
     */
    static void wifiEventHandler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data);
};

} // namespace deiacs