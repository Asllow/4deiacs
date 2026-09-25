#pragma once

#include "esp_err.h"
#include "deiacs_events.h"
#include <cstdarg>
#include <string>
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

namespace deiacs {

class IFunctionBlock;

/**
 * @brief Main Engine and Orchestrator of the 4deiacs Framework.
 *
 * Acts as the "Maestro" of the edge system. Holds the responsibility of
 * instantiating and managing the event bus (native Event Loop) and coordinates
 * the delegated subsystems (SPIFFS, JSON, Registry) to perform safe Hot-Deploy
 * of the real-time control mesh.
 */
class DeiacsEngine {
public:
    /**
     * @brief Initializes the FreeRTOS event bus and telemetry.
     *
     * @return esp_err_t ESP_OK if the engine started successfully.
     */
    static esp_err_t start();

    /**
     * @brief Publishes an event to the internal control bus (IEC 61499).
     *
     * @param event_id ID of the event to be triggered.
     * @param event_data Optional pointer for data transport.
     * @param event_data_size Size of the data payload in bytes.
     * @return esp_err_t ESP_OK on successful enqueue.
     */
    static esp_err_t postEvent(EventIds event_id, void* event_data = nullptr, size_t event_data_size = 0);

    /**
     * @brief Registers an event listener for a Function Block.
     *
     * @param event_id The ID of the event to be monitored.
     * @param event_handler The callback function to be executed.
     * @param event_handler_arg Context pointer (usually the 'this' instance of the block).
     * @return esp_err_t ESP_OK if the subscription was successful.
     */
    static esp_err_t subscribeEvent(EventIds event_id, esp_event_handler_t event_handler, void* event_handler_arg);

    /**
     * @brief Safely deallocates the current mesh to free up RAM.
     *
     * Stops all running blocks, removes their registrations from the native
     * event loop, and frees the allocated memory (SRAM and PSRAM).
     */
    static void clearMesh();

    /**
     * @brief Reloads the control mesh from a networked JSON manifest.
     *
     * @param json_manifest String in PSRAM containing the deployment payload.
     * @return esp_err_t ESP_OK if the mesh was successfully assembled and started.
     */
    static esp_err_t reloadMesh(const char* json_manifest);

    /**
     * @brief Enqueues an event for asynchronous execution in the Dispatcher Task.
     * Breaks the synchronous (depth-first) coupling, resolving the IEC 61499 violation.
     */
    static void enqueueBlockEvent(IFunctionBlock* target, const std::string& port_name);

private:
    static void setupTelemetry();
    static int networkLogRoute(const char* fmt, va_list args);

    static QueueHandle_t s_event_queue;
    static SemaphoreHandle_t s_mesh_mutex;
    static void dispatcherTask(void* pvParameters);
};

} // namespace deiacs