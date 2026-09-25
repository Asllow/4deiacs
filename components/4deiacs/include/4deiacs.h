#pragma once

namespace deiacs {

/**
 * @brief 4deiacs Framework System Entry Point.
 * 
 * Provides a highly abstracted facade to initialize the entire engine, 
 * mount filesystems, connect to the network, and load the persisted mesh.
 */
class System {
public:
    /**
     * @brief Bootstraps the 4deiacs Edge Framework.
     * Blocks until initialization is dispatched.
     */
    static void start();
};

} // namespace deiacs
