/**
 * @file math_node_block.h
 * @brief Generic Math Block header.
 */
#pragma once

#include <string>
#include "i_function_block.h"
#include "cJSON.h"
#include "tinyexpr.h"

namespace deiacs {

/**
 * @brief Generic Math Block (MathNodeBlock).
 * 
 * Capable of processing mathematical equations in real time using TinyExpr.
 * Operates exclusively with float pointers ensuring decoupling.
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

    /* Typed inputs to maintain memory cohesion */
    float* m_in_a;
    float* m_in_b;
    float* m_in_c;
    float* m_in_d;

    /* Internal variables required by the C parser */
    double m_val_a, m_val_b, m_val_c, m_val_d;

    /* Encapsulated output variable */
    float m_out;
};

} /* namespace deiacs */