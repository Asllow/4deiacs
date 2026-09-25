#pragma once

#include <string>
#include <unordered_map>
#include <functional>
#include <vector>
#include "i_function_block.h"
#include "cJSON.h"

namespace deiacs {

/**
 * @brief Block Factory Function Signature.
 * Every block must provide a function with this signature to be dynamically instantiated.
 */
using BlockFactoryFunc = std::function<IFunctionBlock*(const std::string& block_id, cJSON* config)>;

/**
 * @brief Central Registry and Block Factory.
 * 
 * Maintains the mapping of block types to constructors and tracks
 * all active instances in the mesh to allow safe deallocation
 * during the Hot-Deploy process.
 */
class BlockRegistry {
public:
    /**
     * @brief Registers a new block type in the registry.
     *
     * @param block_type The type name (e.g., "AnalogInput").
     * @param factory The function capable of instantiating this block.
     */
    static void registerBlock(const std::string& block_type, BlockFactoryFunc factory);

    /**
     * @brief Instantiates a block dynamically based on its type.
     *
     * @param block_type The block type requested by the JSON.
     * @param block_id The unique ID (instance name) of this block in the mesh.
     * @param config The pointer to the JSON object containing the configurations.
     * @return IFunctionBlock* Pointer to the newly created block (or nullptr on failure).
     */
    static IFunctionBlock* createBlock(const std::string& block_type, const std::string& block_id, cJSON* config);

    /**
     * @brief Destroys all active block instances and clears the tracker.
     * 
     * Invokes the destructor of each instantiated block, ensuring
     * RAM deallocation and the unbinding of peripherals and events.
     */
    static void clearAll();

    /**
     * @brief Replaces the active mesh with a new mesh, ensuring atomicity.
     */
    static void swapInstances(std::vector<IFunctionBlock*>& new_instances);

private:
    /**
     * @brief Singleton to protect the initialization order of the registry in C++.
     */
    static std::unordered_map<std::string, BlockFactoryFunc>& getRegistry();

    /**
     * @brief Singleton to store the pointers of instances created in the current mesh.
     */
    static std::vector<IFunctionBlock*>& getInstances();
};

} // namespace deiacs