#pragma once

#include <string>
#include <vector>
#include "i_function_block.h"
#include "cJSON.h"

namespace Cefet {

/**
 * @brief Gerenciador de Roteamento IEC 61499 (Connection Manager).
 *
 * Responsavel por estabelecer as pontes fisicas de memoria entre os blocos.
 * Cria conexoes de Dados (compartilhamento de ponteiros) e rotas de 
 * Eventos (chamadas de execucao em cascata).
 */
class ConnectionManager {
public:
    /**
     * @brief Executa o roteamento fisico na memoria do microcontrolador.
     *
     * @param conns_array Ponteiro para o array JSON de conexoes ja parseado.
     * @param blocks A lista de blocos ja instanciados pela Factory.
     * @return true Se o roteamento foi processado com sucesso.
     */
    static bool wireConnections(cJSON* conns_array, const std::vector<IFunctionBlock*>& blocks);

    /**
     * @brief Limpa as rotas de rede (Metodo mantido para padronizacao).
     */
    static void clearAll();
};

} // namespace Cefet