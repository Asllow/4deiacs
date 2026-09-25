#include "4deiacs_node_engine.h"
#include "esp_log.h"

#include "block_registry.h"
#include "connection_manager.h"
#include "json_parser.h"
#include "i_function_block.h"
#include <cstring>

namespace Cefet {

struct BlockEventMsg {
    IFunctionBlock* target_block;
    char target_port[16];
};

QueueHandle_t CefetEngine::s_event_queue = nullptr;

ESP_EVENT_DEFINE_BASE(CEFET_CORE_EVENTS);

static const char *TAG = "CEFET_ENGINE";

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

  // Cria a fila do scheduler IEC 61499 (128 eventos pendentes max)
  s_event_queue = xQueueCreate(128, sizeof(BlockEventMsg));
  if (s_event_queue == nullptr) {
      ESP_LOGE(TAG, "Falha critica ao alocar a Event Queue do IEC 61499.");
      return ESP_FAIL;
  }

  // Cria a task despachante central (Scheduler de Eventos) - Alta prioridade para determinismo
  xTaskCreatePinnedToCore(dispatcherTask, "CefetDispatcher", 4096, nullptr, 15, nullptr, 1);

  ESP_LOGI(TAG, "Motor inicializado. Aguardando instrucoes de malha.");
  return ESP_OK;
}

esp_err_t CefetEngine::postEvent(EventIds event_id, void *event_data,
                                 size_t event_data_size) {
  return esp_event_post(CEFET_CORE_EVENTS, event_id, event_data,
                        event_data_size, portMAX_DELAY);
}

esp_err_t CefetEngine::subscribeEvent(EventIds event_id,
                                      esp_event_handler_t event_handler,
                                      void *event_handler_arg) {
  return esp_event_handler_register(CEFET_CORE_EVENTS, event_id, event_handler,
                                    event_handler_arg);
}

void CefetEngine::clearMesh() {
  ESP_LOGI(TAG, "Iniciando destruicao de malha (Hot-Deploy Triggered)...");

  ConnectionManager::clearAll();
  BlockRegistry::clearAll();

  ESP_LOGI(TAG, "Memoria RAM e registos de eventos libertados com sucesso.");
}

esp_err_t CefetEngine::reloadMesh(const char *json_manifest) {
  if (json_manifest == nullptr) {
    ESP_LOGE(TAG, "Payload JSON nulo. Abortando Hot-Deploy.");
    return ESP_FAIL;
  }

  std::vector<IFunctionBlock*> new_mesh;
  esp_err_t parse_result = JsonParser::parseManifest(json_manifest, new_mesh);

  if (parse_result != ESP_OK) {
    ESP_LOGE(TAG,
             "Falha no parsing da nova malha. Abortando Hot-Deploy, mantendo malha anterior.");
    return ESP_FAIL;
  }

  // Atomic Hot-Deploy: only now we swap the mesh
  ESP_LOGI(TAG, "Iniciando destruicao da malha antiga e substituicao...");
  BlockRegistry::swapInstances(new_mesh);

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

int CefetEngine::networkLogRoute(const char *fmt, va_list args) {
  return vprintf(fmt, args);
}

void CefetEngine::enqueueBlockEvent(IFunctionBlock* target, const std::string& port_name) {
    if (s_event_queue == nullptr || target == nullptr) return;

    BlockEventMsg msg;
    msg.target_block = target;
    std::strncpy(msg.target_port, port_name.c_str(), sizeof(msg.target_port) - 1);
    msg.target_port[sizeof(msg.target_port) - 1] = '\0';

    if (xQueueSend(s_event_queue, &msg, 0) != pdTRUE) {
        ESP_LOGE(TAG, "Event Queue CHEIA! Evento %s descartado.", port_name.c_str());
    }
}

void CefetEngine::dispatcherTask(void* pvParameters) {
    BlockEventMsg msg;
    while (true) {
        if (xQueueReceive(s_event_queue, &msg, portMAX_DELAY) == pdTRUE) {
            // Executa o Bloco Funcional de forma assincrona e isolada
            if (msg.target_block != nullptr) {
                msg.target_block->triggerEventInput(msg.target_port);
            }
        }
    }
}

} // namespace Cefet