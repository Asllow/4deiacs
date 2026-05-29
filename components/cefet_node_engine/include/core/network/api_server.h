#pragma once

#include "esp_err.h"
#include <esp_http_server.h>

namespace Cefet {

/**
 * @brief Servidor HTTP local para provisionamento e deploy de malhas IEC 61499.
 *
 * Implementa uma API REST nativa operando na porta 80, responsavel por receber
 * configuracoes de rede e manifestos JSON para atualizacao a quente (Hot-Deploy).
 */
class ApiServer {
public:
    /**
     * @brief Inicializa o servidor HTTP e registra as rotas da API.
     *
     * @return esp_err_t ESP_OK caso o servidor seja iniciado com sucesso.
     */
    static esp_err_t start();

private:
    /**
     * @brief Manipulador da rota POST /provision.
     * * Processa a carga JSON, extrai o novo Hostname, armazena na particao NVS
     * e reinicia o microcontrolador para aplicar as alteracoes no mDNS e Wi-Fi.
     *
     * @param req Ponteiro para a estrutura da requisicao HTTP.
     * @return esp_err_t ESP_OK em caso de sucesso.
     */
    static esp_err_t provisionHandler(httpd_req_t *req);

    /**
     * @brief Manipulador da rota POST /deploy.
     * * Recebe e valida o manifesto JSON contendo a malha de Blocos de Funcao.
     * Implementa recebimento em blocos (chunks) para lidar com payloads extensos.
     *
     * @param req Ponteiro para a estrutura da requisicao HTTP.
     * @return esp_err_t ESP_OK em caso de sucesso.
     */
    static esp_err_t deployHandler(httpd_req_t *req);
};

} // namespace Cefet