/**
 * @file state_space_block.h
 * @brief Bloco Controlador de Espaco de Estados (LQR) Multivariavel.
 */
#pragma once

#include <string>
#include <vector>
#include "i_function_block.h"
#include "cJSON.h"

namespace Cefet {

/**
 * @brief State Space Control Service Interface Function Block (SIFB).
 *
 * Calcula a lei de controlo U = -K * X em tempo real.
 * A matriz de ganhos K e definida no manifesto JSON, permitindo que
 * o bloco se adapte dinamicamente a sistemas SISO (1x1) ou MIMO (MxN)
 * sem necessidade de recompilacao do firmware.
 * * Portas geradas dinamicamente:
 * - Entradas: X_0, X_1, ..., X_n (Ponteiros para os estados atuais)
 * - Saidas: U_0, U_1, ..., U_m (Sinais de atuacao calculados)
 */
class StateSpaceBlock : public IFunctionBlock {
public:
    /**
     * @brief Construtor com alocacao de matriz dinamica.
     * @param block_id Identificador unico do bloco.
     * @param k_matrix Matriz de ganhos K [linhas(U) x colunas(X)].
     */
    StateSpaceBlock(const std::string& block_id, const std::vector<std::vector<float>>& k_matrix);
    
    ~StateSpaceBlock() override;

    bool initialize() override;
    std::string getId() const override;

    void* getDataOutput(const std::string& port_name) override;
    bool connectDataInput(const std::string& port_name, void* data_pointer) override;
    void triggerEventInput(const std::string& event_name) override;

    static IFunctionBlock* create(const std::string& block_id, cJSON* config);

private:
    std::string m_id;
    
    /* Matriz de ganhos do controlador (K) */
    std::vector<std::vector<float>> m_K;
    
    /* Dimensoes descobertas automaticamente a partir do JSON */
    size_t m_num_states;  /* Numero de colunas de K (Vetor X) */
    size_t m_num_outputs; /* Numero de linhas de K (Vetor U) */

    /* Arrays dinamicos de E/S tipados em float (Solid / Polimorfismo) */
    std::vector<float*> m_x_in; 
    std::vector<float> m_u_out; 
};

} /* namespace Cefet */