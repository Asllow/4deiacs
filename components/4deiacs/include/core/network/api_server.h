#pragma once

#include "esp_err.h"
#include "esp_http_server.h"

namespace deiacs {

/**
 * @brief Native HTTP server integrated with LwIP for mesh management.
 *
 * Exposes RESTful endpoints allowing the Graphical User Interface (IDE) to interact
 * with the hardware in a standardized way. Runs in an isolated FreeRTOS Task
 * with reduced priority to avoid interrupting the real-time control cycle.
 */
class ApiServer {
public:
    /**
     * @brief Initializes and starts the HTTP server daemon on port 80.
     *
     * Registers the necessary routes (URIs) for provisioning and deployment.
     *
     * @return esp_err_t ESP_OK on success.
     */
    static esp_err_t start();

    /**
     * @brief Stops the HTTP server and frees network resources.
     */
    static void stop();

private:
    static httpd_handle_t server_handle;

    /**
     * @brief Callback (Handler) for the POST /deploy route.
     *
     * Iteratively allocates the JSON payload in PSRAM, performs syntactic validation,
     * triggers persistence via SpiffsManager, and initiates engine reload.
     *
     * @param req Current HTTP request structure.
     * @return esp_err_t ESP_OK if the flow completes.
     */
    static esp_err_t deployHandler(httpd_req_t *req);
    static esp_err_t provisionHandler(httpd_req_t *req);
};

} // namespace deiacs