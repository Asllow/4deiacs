#pragma once

#include <string>
#include <vector>
#include "i_function_block.h"
#include "cJSON.h"

extern "C" {
    #include "lua.h"
    #include "lualib.h"
    #include "lauxlib.h"
}

namespace deiacs {

/**
 * @brief Sandbox Service Interface Function Block (SIFB).
 *
 * Embeds an isolated Lua interpreter, allocating all its resources 
 * strictly in PSRAM to protect critical SRAM. Supports hybrid 
 * initialization (via Base64 injected into RAM or fallback to SPIFFS files).
 */
class SandboxBlock : public IFunctionBlock {
public:
    /**
     * @brief Hybrid Sandbox constructor.
     * 
     * @param block_id Unique ID of the block in the network.
     * @param script_b64 Base64 encoded Lua source code (Optional).
     * @param script_path SPIFFS fallback path (e.g., "/spiffs/script.lua").
     * @param num_in Number of dynamic input ports.
     * @param num_out Number of dynamic output ports.
     */
    SandboxBlock(const std::string& block_id, const std::string& script_b64, const std::string& script_path, size_t num_in, size_t num_out);

    /**
     * @brief Destroys the isolated VM instance and frees RAM.
     */
    ~SandboxBlock() override;

    /**
     * @brief Initializes the Lua State and invokes the secure code parser.
     * 
     * @return true if compiled successfully.
     */
    bool initialize() override;

    /**
     * @brief Gets the block identification.
     */
    std::string getId() const override;

    /**
     * @brief Returns the pointer to the allocated results (Data Out).
     */
    void* getDataOutput(const std::string& port_name) override;

    /**
     * @brief Binds a data pointer to the dynamic inputs (Data In).
     */
    bool connectDataInput(const std::string& port_name, void* data_pointer) override;

    /**
     * @brief Invokes the Lua VM 'tick' function with safe argument passing.
     */
    void triggerEventInput(const std::string& event_name) override;

    /**
     * @brief Instantiator for BlockRegistry.
     */
    static IFunctionBlock* create(const std::string& block_id, cJSON* config);

private:
    std::string m_id;
    std::string m_script_b64;
    std::string m_script_path;
    size_t m_num_in;
    size_t m_num_out;

    lua_State* L;

    std::vector<float*> m_inputs;
    std::vector<float> m_outputs;

    /**
     * @brief Custom allocator to lock Lua VM in PSRAM.
     */
    static void* lua_psram_alloc(void* ud, void* ptr, size_t osize, size_t nsize);
};

} // namespace deiacs