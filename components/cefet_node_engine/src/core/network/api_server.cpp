#include "api_server.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_system.h"
#include "cJSON.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <cstdlib>
#include <cstring>

namespace Cefet {

static const char* TAG = "API_SERVER";

esp_err_t ApiServer::start() {
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.max_uri_handlers = 4;
    
    httpd_handle_t server = nullptr;
    if (httpd_start(&server, &config) == ESP_OK) {
        httpd_uri_t provision_uri = {
            .uri       = "/provision",
            .method    = HTTP_POST,
            .handler   = provisionHandler,
            .user_ctx  = nullptr
        };
        httpd_register_uri_handler(server, &provision_uri);

        httpd_uri_t deploy_uri = {
            .uri       = "/deploy",
            .method    = HTTP_POST,
            .handler   = deployHandler,
            .user_ctx  = nullptr
        };
        httpd_register_uri_handler(server, &deploy_uri);

        ESP_LOGI(TAG, "Servidor HTTP da API REST iniciado na porta %d", config.server_port);
        return ESP_OK;
    }

    ESP_LOGE(TAG, "Falha ao alocar e iniciar o Servidor HTTP.");
    return ESP_FAIL;
}

esp_err_t ApiServer::provisionHandler(httpd_req_t *req) {
    char content[256];
    size_t recv_size = (req->content_len < sizeof(content)) ? req->content_len : (sizeof(content) - 1);
    
    int ret = httpd_req_recv(req, content, recv_size);
    if (ret <= 0) {
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }
    content[ret] = '\0';

    cJSON *json = cJSON_Parse(content);
    if (json == nullptr) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Payload JSON malformado");
        return ESP_FAIL;
    }

    cJSON *name_item = cJSON_GetObjectItem(json, "name");
    if (cJSON_IsString(name_item) && (name_item->valuestring != nullptr)) {
        nvs_handle_t nvs_handle;
        if (nvs_open("4deacis", NVS_READWRITE, &nvs_handle) == ESP_OK) {
            nvs_set_str(nvs_handle, "deviceName", name_item->valuestring);
            nvs_commit(nvs_handle);
            nvs_close(nvs_handle);
            
            ESP_LOGI(TAG, "Provisionamento concluido. Novo Hostname armazenado: %s", name_item->valuestring);
            httpd_resp_set_type(req, "application/json");
            httpd_resp_sendstr(req, "{\"status\":\"success\", \"message\":\"Provisionado. Reiniciando o motor 4deacis.\"}");
            
            cJSON_Delete(json);
            vTaskDelay(pdMS_TO_TICKS(1000));
            esp_restart();
        }
    }

    cJSON_Delete(json);
    httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Atributo 'name' nao encontrado ou invalido");
    return ESP_FAIL;
}

esp_err_t ApiServer::deployHandler(httpd_req_t *req) {
    char *content = static_cast<char*>(malloc(req->content_len + 1));
    if (content == nullptr) {
        ESP_LOGE(TAG, "Falha de alocacao de memoria para recepcao do manifesto JSON");
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }

    int received = 0;
    while (received < req->content_len) {
        int ret = httpd_req_recv(req, content + received, req->content_len - received);
        if (ret <= 0) {
            if (ret == HTTPD_SOCK_ERR_TIMEOUT) {
                continue;
            }
            free(content);
            httpd_resp_send_500(req);
            return ESP_FAIL;
        }
        received += ret;
    }
    content[received] = '\0';

    ESP_LOGI(TAG, "Manifesto de Deploy recebido com sucesso. Total: %d bytes", received);
    ESP_LOGD(TAG, "Conteudo JSON: %s", content);
    
    httpd_resp_set_type(req, "application/json");
    httpd_resp_sendstr(req, "{\"status\":\"deployed\", \"message\":\"Malha 61499 recebida com sucesso\"}");

    free(content);
    return ESP_OK;
}

} // namespace Cefet