#pragma once

#include "esp_err.h"
#include "cefet_events.h"
#include <cstdarg>
#include <string>
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"

namespace Cefet {

class IFunctionBlock;

/**
 * @brief Motor Principal e Orquestrador do Framework 4deacis.
 *
 * Atua como o "Maestro" do sistema de borda. Mantem a responsabilidade de
 * instanciar e gerir o barramento de eventos (Event Loop nativo) e coordena
 * os subsistemas delegados (SPIFFS, JSON, Registry) para executar o 
 * Hot-Deploy seguro da malha de controlo em tempo real.
 */
class CefetEngine {
public:
    /**
     * @brief Inicializa o barramento de eventos do FreeRTOS e a telemetria.
     *
     * @return esp_err_t ESP_OK se o motor arrancou com sucesso.
     */
    static esp_err_t start();

    /**
     * @brief Publica um evento no barramento interno de controlo (IEC 61499).
     *
     * @param event_id ID do evento a ser disparado.
     * @param event_data Ponteiro opcional para transporte de dados.
     * @param event_data_size Tamanho do payload de dados em bytes.
     * @return esp_err_t ESP_OK em caso de sucesso no enfileiramento.
     */
    static esp_err_t postEvent(EventIds event_id, void* event_data = nullptr, size_t event_data_size = 0);

    /**
     * @brief Regista a escuta de um evento para um Bloco de Funcao.
     *
     * @param event_id O ID do evento a ser monitorizado.
     * @param event_handler A funcao callback a ser executada.
     * @param event_handler_arg Ponteiro de contexto (normalmente a instancia 'this' do bloco).
     * @return esp_err_t ESP_OK se a subscricao foi efetivada.
     */
    static esp_err_t subscribeEvent(EventIds event_id, esp_event_handler_t event_handler, void* event_handler_arg);

    /**
     * @brief Desaloca a malha atual de forma segura para libertacao de RAM.
     *
     * Interrompe todos os blocos em execucao, remove os seus registos no loop
     * de eventos nativo e liberta a memoria alocada (SRAM e PSRAM).
     */
    static void clearMesh();

    /**
     * @brief Recarrega a malha de controlo a partir de um manifesto JSON em rede.
     *
     * @param json_manifest String na PSRAM contendo o payload de deploy.
     * @return esp_err_t ESP_OK se a malha foi montada e iniciada.
     */
    static esp_err_t reloadMesh(const char* json_manifest);

    /**
     * @brief Enfileira um evento para execucao assincrona na Task do Dispatcher.
     * Quebra o acoplamento sincrono (depth-first) resolvendo a violacao IEC 61499.
     */
    static void enqueueBlockEvent(IFunctionBlock* target, const std::string& port_name);

private:
    static void setupTelemetry();
    static int networkLogRoute(const char* fmt, va_list args);

    static QueueHandle_t s_event_queue;
    static void dispatcherTask(void* pvParameters);
};

} // namespace Cefet