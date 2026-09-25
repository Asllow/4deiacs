#include "network_manager.h"
#include "4deiacs_node_engine.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include "esp_mac.h"
#include "nvs_flash.h"
#include "mdns.h"
#include "sdkconfig.h"
#include <cstdio>
#include <cstring>

namespace Cefet {

static const char* TAG = "NETWORK_MANAGER";

/**
 * @brief Buffer estatico para armazenamento do identificador de rede do dispositivo.
 */
static char device_hostname[64] = "4deacis-node";

esp_err_t NetworkManager::connect()
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    nvs_handle_t nvs_handle;
    if (nvs_open("4deacis", NVS_READONLY, &nvs_handle) == ESP_OK) {
        size_t len = sizeof(device_hostname);
        if (nvs_get_str(nvs_handle, "deviceName", device_hostname, &len) != ESP_OK) {
            ESP_LOGW(TAG, "deviceName nao encontrado na NVS. Usando padrao: %s", device_hostname);
        }
        nvs_close(nvs_handle);
    }

    ESP_ERROR_CHECK(esp_netif_init());
    esp_netif_t* netif = esp_netif_create_default_wifi_sta();
    
    esp_netif_set_hostname(netif, device_hostname);

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, 
                                                        ESP_EVENT_ANY_ID, 
                                                        &NetworkManager::wifiEventHandler, 
                                                        nullptr, 
                                                        nullptr));
                                                        
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, 
                                                        IP_EVENT_STA_GOT_IP, 
                                                        &NetworkManager::wifiEventHandler, 
                                                        nullptr, 
                                                        nullptr));

    wifi_config_t wifi_config = {};
    std::strncpy(reinterpret_cast<char*>(wifi_config.sta.ssid), CONFIG_CEFET_WIFI_SSID, sizeof(wifi_config.sta.ssid));
    std::strncpy(reinterpret_cast<char*>(wifi_config.sta.password), CONFIG_CEFET_WIFI_PASS, sizeof(wifi_config.sta.password));
    wifi_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "Subsistema Wi-Fi iniciado. Hostname alvo: %s", device_hostname);
    
    return ESP_OK;
}

void NetworkManager::wifiEventHandler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } 
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        ESP_LOGW(TAG, "Conexao Wi-Fi perdida. Tentando reconectar...");
        esp_wifi_connect();
    } 
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        auto* event = static_cast<ip_event_got_ip_t*>(event_data);
        ESP_LOGI(TAG, "IP atribuido: " IPSTR, IP2STR(&event->ip_info.ip));
        
        static bool mdns_started = false;
        
        if (!mdns_started) {
            if (mdns_init() == ESP_OK) {
                mdns_started = true;
                mdns_hostname_set(device_hostname);
                mdns_instance_name_set(device_hostname);
                mdns_service_add(nullptr, "_http", "_tcp", 80, nullptr, 0);

                uint8_t mac[6];
                esp_wifi_get_mac(WIFI_IF_STA, mac);
                char mac_str[18];
                std::snprintf(mac_str, sizeof(mac_str), "%02X:%02X:%02X:%02X:%02X:%02X", 
                            mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
                
                mdns_service_txt_item_set("_http", "_tcp", "mac", mac_str);
                mdns_service_txt_item_set("_http", "_tcp", "board", CONFIG_IDF_TARGET);
                
                ESP_LOGI(TAG, "mDNS anunciado como: %s.local", device_hostname);
            } else {
                ESP_LOGE(TAG, "Erro ao inicializar mDNS. Descoberta de rede comprometida.");
            }
        }

        CefetEngine::postEvent(EV_NETWORK_CONNECTED);
    }
}

} // namespace Cefet