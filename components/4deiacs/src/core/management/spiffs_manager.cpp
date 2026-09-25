#include "spiffs_manager.h"
#include "esp_spiffs.h"
#include "esp_log.h"
#include "esp_heap_caps.h"
#include <fstream>
#include <sstream>
#include <cstdio>

namespace deiacs {

static const char* TAG = "SPIFFS_MANAGER";
static const char* MESH_FILE_PATH = "/spiffs/mesh.json";

esp_err_t SpiffsManager::mount()
{
    ESP_LOGI(TAG, "Initializing SPIFFS...");

    esp_vfs_spiffs_conf_t conf = {
        .base_path = "/spiffs",
        .partition_label = NULL,
        .max_files = 5,
        .format_if_mount_failed = true
    };

    esp_err_t ret = esp_vfs_spiffs_register(&conf);

    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE(TAG, "Failed to mount or format file system.");
        } else if (ret == ESP_ERR_NOT_FOUND) {
            ESP_LOGE(TAG, "SPIFFS partition not found in partition table.");
        } else {
            ESP_LOGE(TAG, "Failed to initialize SPIFFS (%s)", esp_err_to_name(ret));
        }
        return ret;
    }

    size_t total = 0, used = 0;
    ret = esp_spiffs_info(conf.partition_label, &total, &used);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "Failed to get SPIFFS partition information (%s)", esp_err_to_name(ret));
    } else {
        ESP_LOGI(TAG, "Partition size: total: %d bytes, used: %d bytes", total, used);
    }

    return ESP_OK;
}

void SpiffsManager::unmount()
{
    esp_vfs_spiffs_unregister(NULL);
    ESP_LOGI(TAG, "SPIFFS unmounted.");
}

std::string SpiffsManager::readFile(const std::string& path)
{
    ESP_LOGI(TAG, "Reading file: %s", path.c_str());
    std::ifstream file(path);
    
    if (!file.is_open()) {
        ESP_LOGE(TAG, "Failed to open file: %s", path.c_str());
        return "";
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    file.close();

    return buffer.str();
}

esp_err_t SpiffsManager::saveMesh(const std::string& json_string)
{
    FILE* f = std::fopen(MESH_FILE_PATH, "w");
    if (f == nullptr) {
        ESP_LOGE(TAG, "Failed to open mesh file for writing");
        return ESP_FAIL;
    }

    std::fprintf(f, "%s", json_string.c_str());
    std::fclose(f);

    ESP_LOGI(TAG, "New mesh persisted successfully at %s", MESH_FILE_PATH);
    return ESP_OK;
}

char* SpiffsManager::readMesh()
{
    FILE* f = std::fopen(MESH_FILE_PATH, "r");
    if (f == nullptr) {
        ESP_LOGI(TAG, "No mesh found at %s. Device awaiting deploy.", MESH_FILE_PATH);
        return nullptr;
    }

    std::fseek(f, 0, SEEK_END);
    long size = std::ftell(f);
    std::fseek(f, 0, SEEK_SET);

    char* buffer = static_cast<char*>(heap_caps_malloc(size + 1, MALLOC_CAP_SPIRAM));
    if (buffer != nullptr) {
        std::fread(buffer, 1, size, f);
        buffer[size] = '\0';
        ESP_LOGI(TAG, "Mesh loaded into PSRAM (%ld bytes)", size);
    } else {
        ESP_LOGE(TAG, "Failed to allocate %ld bytes in PSRAM for the mesh", size);
    }

    std::fclose(f);
    return buffer;
}

} // namespace deiacs