#pragma once

#include <string>
#include "esp_err.h"

namespace deiacs {

/**
 * @brief Flash Memory File System Manager (SPIFFS).
 *
 * Responsible for mounting the ESP32 internal data partition,
 * providing standard read methods, and managing the persistence of the
 * dynamic control mesh (JSON) for hot-deploy and state recovery.
 */
class SpiffsManager {
public:
    /**
     * @brief Mounts the SPIFFS partition on the virtual path "/spiffs".
     *
     * @return esp_err_t ESP_OK on success.
     */
    static esp_err_t mount();

    /**
     * @brief Unmounts the SPIFFS partition.
     */
    static void unmount();

    /**
     * @brief Reads the entire content of a generic text file.
     *
     * @param path Absolute file path (e.g., "/spiffs/config.json").
     * @return std::string File content (or empty string on error).
     */
    static std::string readFile(const std::string& path);

    /**
     * @brief Saves the control mesh manifest to non-volatile memory.
     *
     * Overwrites the "/spiffs/mesh.json" file ensuring the device
     * loads the correct mesh on the next initialization cycle.
     *
     * @param json_string JSON payload containing the mesh definition.
     * @return esp_err_t ESP_OK on success, ESP_FAIL otherwise.
     */
    static esp_err_t saveMesh(const std::string& json_string);

    /**
     * @brief Reads the persisted mesh manifest, allocating it directly in PSRAM.
     *
     * @warning The returned pointer must be deallocated by the caller using
     * heap_caps_free() to prevent external memory leaks.
     *
     * @return char* Pointer to the null-terminated C string containing the JSON,
     * or nullptr if the file does not exist.
     */
    static char* readMesh();
};

} // namespace deiacs