#pragma once

#include <string>
#include <vector>
#include <unordered_map>

namespace Cefet {

/**
 * @brief Interface base para todos os Function Blocks do padrao CEFET-61499.
 * * Esta interface garante o polimorfismo do sistema. Qualquer bloco de controle, 
 * I/O de hardware ou interface de rede deve herdar desta classe. Isso permite 
 * que o Gerenciador de Instancias (JSON Parser) trate todos os blocos de forma 
 * generica, alocando-os dinamicamente na memoria RAM do ESP32.
 */
class IFunctionBlock {
protected:
    /**
     * @brief Estrutura interna para mapear o destino de um evento.
     */
    struct EventTarget {
        IFunctionBlock* block;
        std::string port;
    };

    /** @brief Mapa de rotas de eventos de saida (Fios Vermelhos) */
    std::unordered_map<std::string, std::vector<EventTarget>> m_event_routes;

    /**
     * @brief Dispara uma porta de Evento de Saida (Event Out).
     * Acorda imediatamente (Efeito Domino) todos os blocos conectados a esta porta.
     * * @param event_out_name Nome da porta (ex: "CNF" ou "EV_OUT").
     */
    void emitEvent(const std::string& event_out_name) {
        auto it = m_event_routes.find(event_out_name);
        if (it != m_event_routes.end()) {
            for (auto& target : it->second) {
                if (target.block) {
                    target.block->triggerEventInput(target.port);
                }
            }
        }
    }

public:
    /**
     * @brief Destrutor virtual padrao.
     * Garante a liberacao correta de memoria das classes derivadas.
     */
    virtual ~IFunctionBlock() = default;

    /**
     * @brief Rotina de inicializacao do Bloco Funcional.
     * Deve ser chamada logo apos a instanciacao para configurar perifericos
     * (ex: pinos de GPIO) ou alocar recursos de rede.
     * * @return true Se inicializado com sucesso.
     * @return false Se ocorreu falha (ex: hardware nao responde).
     */
    virtual bool initialize() = 0;

    /**
     * @brief Recupera a identificacao unica do bloco instanciado.
     * Este ID e utilizado para criar as conexoes (Wiring) entre os blocos.
     * * @return std::string contendo o ID (ex: "PID_NIVEL_TANQUE_1").
     */
    virtual std::string getId() const = 0;

    // =========================================================================
    // PORTAS DE ROTEAMENTO IEC 61499
    // =========================================================================

    /**
     * @brief Recupera o ponteiro de memoria de uma porta de Saida de Dados (Data Out).
     * * @param port_name Nome da porta na norma (ex: "DATA_OUT").
     * @return void* Ponteiro para a variavel interna do bloco, ou nullptr se nao existir.
     */
    virtual void* getDataOutput(const std::string& port_name) {
        return nullptr; 
    }

    /**
     * @brief Conecta um ponteiro externo a uma porta de Entrada de Dados (Data In).
     * O bloco passara a ler o dado diretamente desta regiao de memoria.
     * * @param port_name Nome da porta na norma (ex: "PAYLOAD_IN").
     * @param data_pointer Ponteiro de memoria originado de outro bloco.
     * @return true Se a porta existe e a conexao foi aceita.
     */
    virtual bool connectDataInput(const std::string& port_name, void* data_pointer) {
        return false;
    }

    /**
     * @brief Aciona uma porta de Entrada de Evento (Event In).
     * Executa a logica interna do bloco associada a este evento.
     * * @param event_name Nome do evento na norma (ex: "REQ" ou "INIT").
     */
    virtual void triggerEventInput(const std::string& event_name) {
        // Implementacao padrao vazia para nao quebrar blocos que ainda nao possuem portas
    }

    /**
     * @brief Registra um 'fio' de evento ligando a saida deste bloco a entrada de outro.
     * * @param event_out_name Nome da porta de saida deste bloco (ex: "CNF").
     * @param target_block Ponteiro para a instancia do bloco de destino na memoria.
     * @param target_port Nome da porta de entrada no bloco de destino (ex: "REQ").
     */
    virtual void connectEventOutput(const std::string& event_out_name, IFunctionBlock* target_block, const std::string& target_port) {
        m_event_routes[event_out_name].push_back({target_block, target_port});
    }
};

} // namespace Cefet