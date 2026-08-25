#pragma once

#include "esp_err.h"
#include "esp_http_server.h"

namespace Cefet {

/**
 * @brief Servidor HTTP nativo integrado ao LwIP para gerenciamento da malha.
 *
 * Expõe endpoints RESTful permitindo que a interface gráfica (IDE) interaja
 * com o hardware de forma padronizada. Opera em uma Task isolada do FreeRTOS
 * com prioridade reduzida para não interromper o ciclo de controle de tempo real.
 */
class ApiServer {
public:
    /**
     * @brief Inicializa e arranca o daemon do servidor HTTP na porta 80.
     *
     * Regista as rotas (URIs) necessarias para o provisionamento e o deploy.
     *
     * @return esp_err_t ESP_OK em caso de sucesso.
     */
    static esp_err_t start();

    /**
     * @brief Encerra o servidor HTTP e liberta os recursos de rede.
     */
    static void stop();

private:
    static httpd_handle_t server_handle;

    /**
     * @brief Callback (Handler) para a rota POST /deploy.
     *
     * Aloca o payload JSON iterativamente na PSRAM, realiza a validacao sintatica,
     * aciona a persistencia via SpiffsManager e dispara o recarregamento do motor.
     *
     * @param req Estrutura da requisicao HTTP atual.
     * @return esp_err_t ESP_OK se o fluxo for concluido.
     */
    static esp_err_t deployHandler(httpd_req_t *req);
    static esp_err_t provisionHandler(httpd_req_t *req);
};

} // namespace Cefet