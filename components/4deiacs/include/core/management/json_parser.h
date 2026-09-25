#pragma once

#include "esp_err.h"
#include <vector>
#include "i_function_block.h"

namespace deiacs {

/**
 * @brief JSON Manifest Parser and Builder.
 *
 * Responsible for deserializing the plain-text manifest received from the network,
 * allocating the cJSON tree in memory only once, instantiating the list of
 * function blocks via BlockRegistry, and invoking the ConnectionManager to
 * establish the physical routing.
 */
class JsonParser {
public:

    /**
     * @brief Parses the manifest, instantiates the blocks, and orchestrates the routing.
     *
     * @param json_payload Character pointer containing the manifest.
     * @param out_instances Vector of blocks that will be populated if parsing succeeds.
     * @return esp_err_t ESP_OK if the mesh was completely assembled and routed.
     */
    static esp_err_t parseManifest(const char* json_payload, std::vector<IFunctionBlock*>& out_instances);
};

} // namespace deiacs