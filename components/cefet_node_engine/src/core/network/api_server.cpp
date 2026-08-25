#include "api_server.h"
#include "esp_log.h"
#include "esp_heap_caps.h"
#include "cJSON.h"
#include "spiffs_manager.h"
#include "cefet_node_engine.h"
#include "network_manager.h"
#include "mdns.h"
#include <string>

namespace Cefet {

static const char* TAG = "API_SERVER";
httpd_handle_t ApiServer::server_handle = nullptr;

// =========================================================================
// HANDLER DA ROTA DE DEPLOY DE MALHA (/deploy)
// =========================================================================
esp_err_t ApiServer::deployHandler(httpd_req_t *req)
{
    if (req->content_len <= 0) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Payload vazio");
        return ESP_FAIL;
    }

    char* json_buf = static_cast<char*>(heap_caps_malloc(req->content_len + 1, MALLOC_CAP_SPIRAM));
    if (json_buf == nullptr) {
        ESP_LOGE(TAG, "Falha de alocacao na PSRAM para o payload JSON");
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Memoria PSRAM insuficiente");
        return ESP_FAIL;
    }

    int received = 0;
    int remaining = req->content_len;
    
    while (remaining > 0) {
        int ret = httpd_req_recv(req, json_buf + received, remaining);
        if (ret <= 0) {
            if (ret == HTTPD_SOCK_ERR_TIMEOUT) {
                continue; 
            }
            heap_caps_free(json_buf);
            return ESP_FAIL;
        }
        received += ret;
        remaining -= ret;
    }
    json_buf[req->content_len] = '\0';

    cJSON* root = cJSON_Parse(json_buf);
    if (root == nullptr) {
        ESP_LOGE(TAG, "Falha no parse do JSON. Payload ignorado.");
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Sintaxe JSON invalida");
        heap_caps_free(json_buf);
        return ESP_FAIL;
    }
    cJSON_Delete(root);

    std::string json_str(json_buf);
    
    if (SpiffsManager::saveMesh(json_str) != ESP_OK) {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Falha ao gravar na Flash");
        heap_caps_free(json_buf);
        return ESP_FAIL;
    }

    CefetEngine::reloadMesh(json_buf);

    heap_caps_free(json_buf);

    const char* resp = "{\"status\":\"deployed\",\"message\":\"Malha injetada e persistida com sucesso\"}";
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, resp, HTTPD_RESP_USE_STRLEN);

    return ESP_OK;
}

// =========================================================================
// HANDLER DA ROTA DE PROVISIONAMENTO (/provision)
// =========================================================================
esp_err_t ApiServer::provisionHandler(httpd_req_t *req)
{
    char buf[256];
    int remaining = req->content_len;

    if (remaining >= sizeof(buf)) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Payload muito grande para provisionamento");
        return ESP_FAIL;
    }

    // Le a string do payload
    int ret = httpd_req_recv(req, buf, remaining);
    if (ret <= 0) {
        if (ret == HTTPD_SOCK_ERR_TIMEOUT) {
            httpd_resp_send_408(req);
        }
        return ESP_FAIL;
    }
    buf[ret] = '\0';

    cJSON *root = cJSON_Parse(buf);
    if (root == nullptr) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "JSON Invalido");
        return ESP_FAIL;
    }

    cJSON *name_item = cJSON_GetObjectItem(root, "name");
    if (cJSON_IsString(name_item) && (name_item->valuestring != nullptr)) {
        
        std::string novo_nome = name_item->valuestring;
        ESP_LOGI(TAG, "Pedido de Provisionamento recebido. Novo hostname: %s", novo_nome.c_str());

        // 1. Salva fisicamente na Flash (para sobreviver a reboots)
        FILE* f = fopen("/spiffs/device.json", "w");
        if (f) {
            fprintf(f, "{\"name\":\"%s\"}", novo_nome.c_str());
            fclose(f);
        } else {
            ESP_LOGE(TAG, "Falha ao abrir /spiffs/device.json para escrita.");
        }

        // 2. Altera dinamicamente o mDNS na hora
        mdns_hostname_set(novo_nome.c_str());
        mdns_instance_name_set(novo_nome.c_str());

        // Responde de volta a WebIDE
        const char* resp_str = "{\"status\":\"provisioned\"}";
        httpd_resp_set_type(req, "application/json");
        httpd_resp_send(req, resp_str, HTTPD_RESP_USE_STRLEN);

    } else {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Campo 'name' ausente no JSON");
    }

    cJSON_Delete(root);
    return ESP_OK;
}

// =========================================================================
// CONTROLE DO SERVIDOR
// =========================================================================
esp_err_t ApiServer::start()
{
    if (server_handle != nullptr) {
        return ESP_OK;
    }

    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.max_uri_handlers = 8;
    config.max_req_hdr_len = 1024;
    config.core_id = 0; 
    
    ESP_LOGI(TAG, "A iniciar servidor HTTP na porta %d", config.server_port);

    if (httpd_start(&server_handle, &config) == ESP_OK) {
        
        // Rota 1: Injeção da Malha de Controle
        httpd_uri_t uri_deploy = {
            .uri       = "/deploy",
            .method    = HTTP_POST,
            .handler   = deployHandler,
            .user_ctx  = nullptr
        };
        httpd_register_uri_handler(server_handle, &uri_deploy);
        
        // Rota 2: Renomear a placa na Rede
        httpd_uri_t uri_provision = {
            .uri       = "/provision",
            .method    = HTTP_POST,
            .handler   = provisionHandler,
            .user_ctx  = nullptr
        };
        httpd_register_uri_handler(server_handle, &uri_provision);

        return ESP_OK;
    }

    ESP_LOGE(TAG, "Falha ao iniciar o servidor HTTP");
    return ESP_FAIL;
}

void ApiServer::stop()
{
    if (server_handle != nullptr) {
        httpd_stop(server_handle);
        server_handle = nullptr;
        ESP_LOGI(TAG, "Servidor HTTP parado");
    }
}

} // namespace Cefet