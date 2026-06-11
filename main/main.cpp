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
    ESP_LOGI(TAG, "Iniciando Node CEFET-61499 (4deacis)...");

    // 1. Inicia o Barramento de Eventos (Coração do Motor)
    Cefet::CefetEngine::start();

    // 2. Monta o Sistema de Arquivos (Memória Flash)
    Cefet::SpiffsManager::mount();

    // 3. Conecta no Wi-Fi e anuncia no mDNS
    Cefet::NetworkManager::connect();

    // 4. Inicia o servidor REST na porta 80 para escutar a IDE Web
    Cefet::ApiServer::start();

    // 5. Tenta reviver a malha da inicialização anterior (Cold Boot)
    char* saved_mesh = Cefet::SpiffsManager::readMesh();
    if (saved_mesh != nullptr) {
        ESP_LOGI(TAG, "Malha persistente encontrada. A inicializar...");
        
        // Recarrega a malha e roteia os fios
        Cefet::CefetEngine::reloadMesh(saved_mesh);
        
        // IMPORTANTE: Como SpiffsManager::readMesh() alocou o arquivo na PSRAM, 
        // precisamos liberar a memória após o motor instanciar os blocos!
        heap_caps_free(saved_mesh);
    } else {
        ESP_LOGI(TAG, "Nenhuma malha anterior encontrada. Placa em modo IDLE aguardando Deploy.");
    }

    // A Thread principal (app_main) já fez o seu trabalho de orquestração.
    // Agora o FreeRTOS e os Timers assumem o controle. Podemos deletar esta Task.
    vTaskDelete(NULL);
}