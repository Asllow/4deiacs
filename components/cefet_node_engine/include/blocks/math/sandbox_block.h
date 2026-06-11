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

namespace Cefet {

/**
 * @brief Sandbox Service Interface Function Block (SIFB).
 *
 * Embute um interpretador Lua isolado, alocando todos os seus recursos 
 * estritamente na PSRAM para proteger a SRAM critica. Suporta inicializacao
 * hibrida (via Base64 injetado na RAM ou fallback para ficheiros no SPIFFS).
 */
class SandboxBlock : public IFunctionBlock {
public:
    /**
     * @brief Construtor do Sandbox hibrido.
     * * @param block_id ID unico do bloco na rede.
     * @param script_b64 Codigo fonte Lua codificado em Base64 (Opcional).
     * @param script_path Caminho de fallback no SPIFFS (Ex: "/spiffs/script.lua").
     * @param num_in Quantidade de portas de entrada dinâmicas.
     * @param num_out Quantidade de portas de saida dinâmicas.
     */
    SandboxBlock(const std::string& block_id, const std::string& script_b64, const std::string& script_path, size_t num_in, size_t num_out);

    /**
     * @brief Destroi a instância isolada da VM e liberta a RAM.
     */
    ~SandboxBlock() override;

    /**
     * @brief Inicializa o Estado Lua e invoca o parser de codigo seguro.
     * * @return true se compilado com sucesso.
     */
    bool initialize() override;

    /**
     * @brief Obtem a identificacao do bloco.
     */
    std::string getId() const override;

    /**
     * @brief Retorna o ponteiro para os resultados alocados (Data Out).
     */
    void* getDataOutput(const std::string& port_name) override;

    /**
     * @brief Vincula um ponteiro de dados as entradas dinamicas (Data In).
     */
    bool connectDataInput(const std::string& port_name, void* data_pointer) override;

    /**
     * @brief Invoca a funcao 'tick' da VM Lua com passagem segura de argumentos.
     */
    void triggerEventInput(const std::string& event_name) override;

    /**
     * @brief Instanciador para o BlockRegistry.
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
     * @brief Alocador customizado para travar a VM Lua na PSRAM.
     */
    static void* lua_psram_alloc(void* ud, void* ptr, size_t osize, size_t nsize);
};

} // namespace Cefet