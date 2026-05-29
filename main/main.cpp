#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "cefet_node_engine.h"
#include "network_manager.h"
#include "api_server.h"
#include "esp_heap_caps.h"
#include "esp_flash.h"
#include "esp_log.h"

extern "C" void app_main(void)
{
    uint32_t flash_size = 0;
    esp_flash_get_size(NULL, &flash_size);
    ESP_LOGI("MEMORIA", "=== RELATORIO DE HARDWARE ===");
    ESP_LOGI("MEMORIA", "Flash Total: %lu MB", flash_size / (1024 * 1024));
    ESP_LOGI("MEMORIA", "RAM Interna Livre: %zu bytes", heap_caps_get_free_size(MALLOC_CAP_INTERNAL));
    ESP_LOGI("MEMORIA", "PSRAM Externa Livre: %zu bytes", heap_caps_get_free_size(MALLOC_CAP_SPIRAM));
    ESP_LOGI("MEMORIA", "=============================");

    Cefet::CefetEngine::start();
    Cefet::NetworkManager::connect();
    
    Cefet::ApiServer::start();

    Cefet::CefetEngine::startFromManifest("/spiffs/config.json");

    while (1) {
        vTaskDelay(portMAX_DELAY);
    }
}