#include "sandbox_block.h"
#include "block_registry.h"
#include "esp_log.h"
#include "esp_heap_caps.h"
#include "mbedtls/base64.h"

namespace Cefet {

static const char* TAG = "SANDBOX_LUA";

void* SandboxBlock::lua_psram_alloc(void* ud, void* ptr, size_t osize, size_t nsize) {
    (void)ud; 
    (void)osize;
    if (nsize == 0) {
        heap_caps_free(ptr);
        return nullptr;
    }
    return heap_caps_realloc(ptr, nsize, MALLOC_CAP_SPIRAM);
}

SandboxBlock::SandboxBlock(const std::string& block_id, const std::string& script_b64, const std::string& script_path, size_t num_in, size_t num_out)
    : m_id(block_id), m_script_b64(script_b64), m_script_path(script_path), m_num_in(num_in), m_num_out(num_out), L(nullptr)
{
    m_inputs.resize(m_num_in, nullptr);
    m_outputs.resize(m_num_out, 0.0f);
}

SandboxBlock::~SandboxBlock()
{
    if (L != nullptr) {
        lua_close(L);
    }
}

bool SandboxBlock::initialize()
{
    L = lua_newstate(SandboxBlock::lua_psram_alloc, nullptr);
    if (L == nullptr) {
        ESP_LOGE(TAG, "[%s] Impossivel alocar PSRAM para a VM Lua.", m_id.c_str());
        return false;
    }

    luaL_openlibs(L);

    if (!m_script_b64.empty()) {
        ESP_LOGI(TAG, "[%s] Decodificando script Base64...", m_id.c_str());
        
        size_t b64_len = m_script_b64.length();
        unsigned char* decoded_buffer = static_cast<unsigned char*>(heap_caps_malloc(b64_len + 1, MALLOC_CAP_SPIRAM));

        if (decoded_buffer == nullptr) {
            ESP_LOGE(TAG, "[%s] Falha ao alocar buffer Base64 na PSRAM.", m_id.c_str());
            return false;
        }

        size_t actual_out_len = 0;
        int ret = mbedtls_base64_decode(decoded_buffer, b64_len, &actual_out_len,
                                        reinterpret_cast<const unsigned char*>(m_script_b64.c_str()), b64_len);

        if (ret == 0) {
            decoded_buffer[actual_out_len] = '\0';
            
            if (luaL_dostring(L, reinterpret_cast<const char*>(decoded_buffer)) != LUA_OK) {
                ESP_LOGE(TAG, "[%s] Erro Sintatico Lua: %s", m_id.c_str(), lua_tostring(L, -1));
                lua_pop(L, 1);
                heap_caps_free(decoded_buffer);
                return false;
            }
        } else {
            ESP_LOGE(TAG, "[%s] Falha mbedtls_base64_decode: %d", m_id.c_str(), ret);
            heap_caps_free(decoded_buffer);
            return false;
        }
        heap_caps_free(decoded_buffer);
    } 
    else {
        ESP_LOGI(TAG, "[%s] Carregando fallback via SPIFFS: %s", m_id.c_str(), m_script_path.c_str());
        if (luaL_dofile(L, m_script_path.c_str()) != LUA_OK) {
            ESP_LOGE(TAG, "[%s] Erro I/O ou compilacao SPIFFS: %s", m_id.c_str(), lua_tostring(L, -1));
            lua_pop(L, 1);
            return false;
        }
    }

    ESP_LOGI(TAG, "[%s] VM pronta. Portas IN: %zu | OUT: %zu", m_id.c_str(), m_num_in, m_num_out);
    return true;
}

std::string SandboxBlock::getId() const
{
    return m_id;
}

void* SandboxBlock::getDataOutput(const std::string& port_name)
{
    if (port_name.find("OUT_") == 0) {
        size_t idx = std::stoi(port_name.substr(4));
        if (idx < m_num_out) {
            return &m_outputs[idx];
        }
    }
    return nullptr;
}

bool SandboxBlock::connectDataInput(const std::string& port_name, void* data_pointer)
{
    if (port_name.find("IN_") == 0) {
        size_t idx = std::stoi(port_name.substr(3));
        if (idx < m_num_in) {
            m_inputs[idx] = static_cast<float*>(data_pointer);
            return true;
        }
    }
    return false;
}

void SandboxBlock::triggerEventInput(const std::string& event_name)
{
    if (event_name == "REQ" && L != nullptr) {
        lua_getglobal(L, "tick");

        if (!lua_isfunction(L, -1)) {
            ESP_LOGW(TAG, "[%s] Rotina 'tick' ausente no script.", m_id.c_str());
            lua_pop(L, 1);
            return;
        }

        for (size_t i = 0; i < m_num_in; ++i) {
            float val = (m_inputs[i] != nullptr) ? *(m_inputs[i]) : 0.0f;
            lua_pushnumber(L, val);
        }

        if (lua_pcall(L, m_num_in, m_num_out, 0) != LUA_OK) {
            ESP_LOGE(TAG, "[%s] Runtime Panic em tick(): %s", m_id.c_str(), lua_tostring(L, -1));
            lua_pop(L, 1);
            return;
        }

        for (int i = static_cast<int>(m_num_out) - 1; i >= 0; --i) {
            m_outputs[i] = static_cast<float>(lua_tonumber(L, -1));
            lua_pop(L, 1);
        }

        emitEvent("CNF");
    }
}

IFunctionBlock* SandboxBlock::create(const std::string& block_id, cJSON* config)
{
    std::string script_b64 = "";
    std::string script_path = "/spiffs/script.lua"; 
    size_t in_ports = 4;  
    size_t out_ports = 4; 
    
    if (config != nullptr) {
        cJSON* b64_item = cJSON_GetObjectItem(config, "script_b64");
        if (cJSON_IsString(b64_item) && b64_item->valuestring != nullptr) {
            script_b64 = b64_item->valuestring;
        }

        cJSON* path_item = cJSON_GetObjectItem(config, "script_path");
        if (cJSON_IsString(path_item) && path_item->valuestring != nullptr) {
            script_path = path_item->valuestring;
        }

        cJSON* in_item = cJSON_GetObjectItem(config, "num_in");
        if (cJSON_IsNumber(in_item)) {
            in_ports = static_cast<size_t>(in_item->valueint);
        }

        cJSON* out_item = cJSON_GetObjectItem(config, "num_out");
        if (cJSON_IsNumber(out_item)) {
            out_ports = static_cast<size_t>(out_item->valueint);
        }
    }
    
    return new SandboxBlock(block_id, script_b64, script_path, in_ports, out_ports);
}

static bool registered = []() {
    BlockRegistry::registerBlock("Sandbox", SandboxBlock::create);
    return true;
}();

} // namespace Cefet