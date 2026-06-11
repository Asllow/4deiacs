#pragma once

#include <string>
#include <unordered_map>
#include <functional>
#include <vector>
#include "i_function_block.h"
#include "cJSON.h"

namespace Cefet {

/**
 * @brief Assinatura da funcao fabrica (Factory Function).
 * Todo bloco deve prover uma funcao com esta assinatura para ser criado dinamicamente.
 */
using BlockFactoryFunc = std::function<IFunctionBlock*(const std::string& block_id, cJSON* config)>;

/**
 * @brief Registro Central e Fabrica de Blocos.
 * * Mantem o mapeamento de tipos de blocos para construtores e rastreia
 * todas as instancias ativas na malha para permitir a desalocacao
 * segura durante o processo de Hot-Deploy.
 */
class BlockRegistry {
public:
    /**
     * @brief Registra um novo tipo de bloco no dicionario.
     *
     * @param block_type O nome do tipo (ex: "AnalogInput").
     * @param factory A funcao que sabe instanciar este bloco.
     */
    static void registerBlock(const std::string& block_type, BlockFactoryFunc factory);

    /**
     * @brief Instancia um bloco dinamicamente baseado no seu tipo.
     *
     * @param block_type O tipo do bloco solicitado pelo JSON.
     * @param block_id O ID unico (nome da instancia) deste bloco na rede.
     * @param config O ponteiro para o pedaco do JSON que contem as configuracoes.
     * @return IFunctionBlock* Ponteiro para o bloco recem-criado (ou nullptr se falhar).
     */
    static IFunctionBlock* createBlock(const std::string& block_type, const std::string& block_id, cJSON* config);

    /**
     * @brief Destroi todas as instancias de blocos ativas e limpa o rastreador.
     * * Invoca o destrutor (delete) de cada bloco instanciado, garantindo a
     * libertacao de memoria RAM e o desvinculo de perifericos e eventos.
     */
    static void clearAll();

private:
    /**
     * @brief Singleton para proteger a ordem de inicializacao do dicionario em C++.
     */
    static std::unordered_map<std::string, BlockFactoryFunc>& getRegistry();

    /**
     * @brief Singleton para armazenar os ponteiros das instancias criadas na malha atual.
     */
    static std::vector<IFunctionBlock*>& getInstances();
};

} // namespace Cefet