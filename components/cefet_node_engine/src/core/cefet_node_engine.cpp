#include "cefet_node_engine.h"
#include "esp_log.h"
#include "sdkconfig.h"

#include "spiffs_manager.h"
#include "json_parser.h"
#include "connection_manager.h"
#include "block_registry.h"

namespace Cefet {

ESP_EVENT_DEFINE_BASE(CEFET_CORE_EVENTS);

static const char* TAG = "CEFET_ENGINE";

esp_err_t CefetEngine::start() {
    setupTelemetry();

    ESP_LOGI(TAG, "A inicializar o Motor de Eventos IEC-61499 (4deacis)...");

    esp_err_t err = esp_event_loop_create_default();
    if (err != ESP_OK) {
        if (err == ESP_ERR_INVALID_STATE) {
            ESP_LOGW(TAG, "Event Loop nativo ja se encontrava em execucao.");
        } else {
            ESP_LOGE(TAG, "Falha critica ao alocar o Event Loop no FreeRTOS.");
            return err;
        }
    }

    ESP_LOGI(TAG, "Motor inicializado. Aguardando instrucoes de malha.");
    return ESP_OK;
}

esp_err_t CefetEngine::postEvent(EventIds event_id, void* event_data, size_t event_data_size) {
    return esp_event_post(CEFET_CORE_EVENTS, event_id, event_data, event_data_size, portMAX_DELAY);
}

esp_err_t CefetEngine::subscribeEvent(EventIds event_id, esp_event_handler_t event_handler, void* event_handler_arg) {
    return esp_event_handler_register(CEFET_CORE_EVENTS, event_id, event_handler, event_handler_arg);
}

void CefetEngine::clearMesh() {
    ESP_LOGI(TAG, "Iniciando destruicao de malha (Hot-Deploy Triggered)...");

    ConnectionManager::clearAll();
    BlockRegistry::clearAll();

    ESP_LOGI(TAG, "Memoria RAM e registos de eventos libertados com sucesso.");
}

esp_err_t CefetEngine::reloadMesh(const char* json_manifest) {
    if (json_manifest == nullptr) {
        ESP_LOGE(TAG, "Payload JSON nulo. Abortando Hot-Deploy.");
        return ESP_FAIL;
    }

    clearMesh();

    esp_err_t parse_result = JsonParser::parseManifest(json_manifest);

    if (parse_result != ESP_OK) {
        ESP_LOGE(TAG, "Falha no parsing da nova malha. O dispositivo entrou em IDLE.");
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "Hot-Deploy concluido! Sistema a operar com nova topologia.");
    return ESP_OK;
}

void CefetEngine::setupTelemetry() {
#if defined(CONFIG_CEFET_LOG_MODE_DISABLED)
    esp_log_level_set("*", ESP_LOG_NONE);
#elif defined(CONFIG_CEFET_LOG_MODE_NETWORK)
    esp_log_set_vprintf(&CefetEngine::networkLogRoute);
#endif
}

int CefetEngine::networkLogRoute(const char* fmt, va_list args) {
    return vprintf(fmt, args);
}

} // namespace Cefet