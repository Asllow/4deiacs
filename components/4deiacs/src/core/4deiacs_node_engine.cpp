#include "4deiacs_node_engine.h"
#include "esp_log.h"

#include "block_registry.h"
#include "connection_manager.h"
#include "json_parser.h"
#include "i_function_block.h"
#include <cstring>

namespace deiacs {

struct BlockEventMsg {
    IFunctionBlock* target_block;
    char target_port[16];
};

QueueHandle_t DeiacsEngine::s_event_queue = nullptr;
SemaphoreHandle_t DeiacsEngine::s_mesh_mutex = nullptr;

ESP_EVENT_DEFINE_BASE(DEIACS_CORE_EVENTS);

static const char *TAG = "DEIACS_ENGINE";

esp_err_t DeiacsEngine::start() {
  setupTelemetry();

  ESP_LOGI(TAG, "Initializing IEC-61499 Event Engine (4deiacs)...");

  esp_err_t err = esp_event_loop_create_default();
  if (err != ESP_OK) {
    if (err == ESP_ERR_INVALID_STATE) {
      ESP_LOGW(TAG, "Native Event Loop was already running.");
    } else {
      ESP_LOGE(TAG, "Critical failure allocating Event Loop in FreeRTOS.");
      return err;
    }
  }


  s_event_queue = xQueueCreate(128, sizeof(BlockEventMsg));
  if (s_event_queue == nullptr) {
      ESP_LOGE(TAG, "Critical failure allocating IEC 61499 Event Queue.");
      return ESP_FAIL;
  }

  s_mesh_mutex = xSemaphoreCreateMutex();
  if (s_mesh_mutex == nullptr) {
      ESP_LOGE(TAG, "Critical failure allocating the Mesh Mutex.");
      return ESP_FAIL;
  }


  xTaskCreatePinnedToCore(dispatcherTask, "4deiacsDispatch", 4096, nullptr, 15, nullptr, 1);

  ESP_LOGI(TAG, "Engine initialized. Awaiting mesh instructions.");
  return ESP_OK;
}

esp_err_t DeiacsEngine::postEvent(EventIds event_id, void *event_data,
                                 size_t event_data_size) {
  return esp_event_post(DEIACS_CORE_EVENTS, event_id, event_data,
                        event_data_size, portMAX_DELAY);
}

esp_err_t DeiacsEngine::subscribeEvent(EventIds event_id,
                                      esp_event_handler_t event_handler,
                                      void *event_handler_arg) {
  return esp_event_handler_register(DEIACS_CORE_EVENTS, event_id, event_handler,
                                    event_handler_arg);
}

void DeiacsEngine::clearMesh() {
  ESP_LOGI(TAG, "Initiating mesh destruction (Hot-Deploy Triggered)...");

  ConnectionManager::clearAll();
  BlockRegistry::clearAll();

  ESP_LOGI(TAG, "RAM memory and event registries freed successfully.");
}

esp_err_t DeiacsEngine::reloadMesh(const char *json_manifest) {
  if (json_manifest == nullptr) {
    ESP_LOGE(TAG, "Null JSON payload. Aborting Hot-Deploy.");
    return ESP_FAIL;
  }

  std::vector<IFunctionBlock*> new_mesh;
  esp_err_t parse_result = JsonParser::parseManifest(json_manifest, new_mesh);

  if (parse_result != ESP_OK) {
    ESP_LOGE(TAG,
             "Failed to parse new mesh. Aborting Hot-Deploy, keeping previous mesh.");
    return ESP_FAIL;
  }


  ESP_LOGI(TAG, "Initiating old mesh destruction and replacement...");
  
  if (xSemaphoreTake(s_mesh_mutex, portMAX_DELAY) == pdTRUE) {
      xQueueReset(s_event_queue);
      BlockRegistry::swapInstances(new_mesh);
      xSemaphoreGive(s_mesh_mutex);
  }

  ESP_LOGI(TAG, "Hot-Deploy complete! System operating with new topology.");
  return ESP_OK;
}

void DeiacsEngine::setupTelemetry() {
#if defined(CONFIG_DEIACS_LOG_MODE_DISABLED)
  esp_log_level_set("*", ESP_LOG_NONE);
#elif defined(CONFIG_DEIACS_LOG_MODE_NETWORK)
  esp_log_set_vprintf(&DeiacsEngine::networkLogRoute);
#endif
}

int DeiacsEngine::networkLogRoute(const char *fmt, va_list args) {
  return vprintf(fmt, args);
}

void DeiacsEngine::enqueueBlockEvent(IFunctionBlock* target, const std::string& port_name) {
    if (s_event_queue == nullptr || target == nullptr) return;

    BlockEventMsg msg;
    msg.target_block = target;
    std::strncpy(msg.target_port, port_name.c_str(), sizeof(msg.target_port) - 1);
    msg.target_port[sizeof(msg.target_port) - 1] = '\0';

    if (xQueueSend(s_event_queue, &msg, 0) != pdTRUE) {
        ESP_LOGE(TAG, "Event Queue FULL! Event %s discarded.", port_name.c_str());
    }
}

void DeiacsEngine::dispatcherTask(void* pvParameters) {
    BlockEventMsg msg;
    while (true) {
        if (xQueueReceive(s_event_queue, &msg, portMAX_DELAY) == pdTRUE) {

            if (msg.target_block != nullptr) {
                if (xSemaphoreTake(s_mesh_mutex, portMAX_DELAY) == pdTRUE) {
                    msg.target_block->triggerEventInput(msg.target_port);
                    xSemaphoreGive(s_mesh_mutex);
                }
            }
        }
    }
}

} // namespace deiacs