#pragma once

#include <string>
#include <vector>
#include "i_function_block.h"
#include "cJSON.h"

namespace deiacs {

/**
 * @brief IEC 61499 Routing Manager (Connection Manager).
 *
 * Responsible for establishing the physical memory bridges between blocks.
 * Creates Data connections (pointer sharing) and Event routes 
 * (cascading execution calls).
 */
class ConnectionManager {
public:
    /**
     * @brief Executes the physical routing in the microcontroller's memory.
     *
     * @param conns_array Pointer to the parsed JSON connections array.
     * @param blocks The list of blocks already instantiated by the Factory.
     * @return true If the routing was processed successfully.
     */
    static bool wireConnections(cJSON* conns_array, const std::vector<IFunctionBlock*>& blocks);

    /**
     * @brief Clears network routes (Method kept for standardization).
     */
    static void clearAll();
};

} // namespace deiacs