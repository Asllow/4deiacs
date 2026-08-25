#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "cefet_node_engine.h"
#include "network_manager.h"
#include "api_server.h"
#include "spiffs_manager.h"
#include "esp_log.h"
#include "esp_heap_caps.h"

static const char* TAG = "APP_MAIN";

extern "C" void app_main() {
    ESP_LOGI(TAG, "Iniciando 4deiacs...");

    Cefet::CefetEngine::start();

    Cefet::SpiffsManager::mount();

    Cefet::NetworkManager::connect();

    Cefet::ApiServer::start();

    char* saved_mesh = Cefet::SpiffsManager::readMesh();
    if (saved_mesh != nullptr) {
        ESP_LOGI(TAG, "Malha persistente encontrada. A inicializar...");
        Cefet::CefetEngine::reloadMesh(saved_mesh);
        heap_caps_free(saved_mesh);
    } else {
        ESP_LOGI(TAG, "Nenhuma malha anterior encontrada. Placa em modo IDLE aguardando Deploy.");
    }
    vTaskDelete(NULL);
}