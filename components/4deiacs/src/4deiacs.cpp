#include "4deiacs.h"
#include "4deiacs_node_engine.h"
#include "api_server.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "network_manager.h"
#include "spiffs_manager.h"

namespace deiacs {

static const char *TAG = "4DEIACS_SYSTEM";

void System::start() {
  ESP_LOGI(TAG, "Starting 4deiacs Framework...");

  DeiacsEngine::start();
  SpiffsManager::mount();
  NetworkManager::connect();
  ApiServer::start();

  char *saved_mesh = SpiffsManager::readMesh();
  if (saved_mesh != nullptr) {
    ESP_LOGI(TAG, "Persistent mesh found. Initializing...");
    DeiacsEngine::reloadMesh(saved_mesh);
    heap_caps_free(saved_mesh);
  } else {
    ESP_LOGI(TAG, "No previous mesh found. Board in IDLE mode awaiting Deploy.");
  }
}

} // namespace deiacs
