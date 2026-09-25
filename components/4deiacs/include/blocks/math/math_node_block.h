/**
 * @file math_node_block.h
 * @brief Cabecalho do Bloco Matematico Generico.
 */
#pragma once

#include <string>
#include "i_function_block.h"
#include "cJSON.h"
#include "tinyexpr.h"

namespace Cefet {

/**
 * @brief Bloco Matematico Generico (MathNodeBlock).
 * * Capaz de processar equacoes matematicas em tempo real utilizando a TinyExpr.
 * Opera exclusivamente com ponteiros float garantindo o desacoplamento.
 */
class MathNodeBlock : public IFunctionBlock {
public:
    MathNodeBlock(const std::string& block_id, const std::string& expression);
    ~MathNodeBlock() override;

    bool initialize() override;
    std::string getId() const override;

    void* getDataOutput(const std::string& port_name) override;
    bool connectDataInput(const std::string& port_name, void* data_pointer) override;
    void triggerEventInput(const std::string& event_name) override;

    static IFunctionBlock* create(const std::string& block_id, cJSON* config);

private:
    std::string m_id;
    std::string m_expression;
    te_expr* m_compiled_expr;

    /* Entradas tipadas para manter a coesao de memoria */
    float* m_in_a;
    float* m_in_b;
    float* m_in_c;
    float* m_in_d;

    /* Variaveis internas exigidas pelo parser C */
    double m_val_a, m_val_b, m_val_c, m_val_d;

    /* Variavel de saida encapsulada */
    float m_out;
};

} /* namespace Cefet */