/**
 * @file math_node_block.cpp
 * @brief Implementacao do Bloco Matematico com parser TinyExpr.
 */
#include "math_node_block.h"
#include "block_registry.h"
#include "esp_log.h"

namespace Cefet {

static const char* TAG = "MATH_BLOCK";

MathNodeBlock::MathNodeBlock(const std::string& block_id, const std::string& expression)
    : m_id(block_id), m_expression(expression), m_compiled_expr(nullptr),
      m_in_a(nullptr), m_in_b(nullptr), m_in_c(nullptr), m_in_d(nullptr),
      m_val_a(0.0), m_val_b(0.0), m_val_c(0.0), m_val_d(0.0), m_out(0.0f) 
{
}

MathNodeBlock::~MathNodeBlock() 
{
    if (m_compiled_expr != nullptr) {
        te_free(m_compiled_expr);
    }
}

bool MathNodeBlock::initialize() 
{
    /* Declaracao exaustiva dos parametros exigidos pelo C moderno */
    te_variable vars[] = {
        {"IN_A", &m_val_a, 0, nullptr},
        {"IN_B", &m_val_b, 0, nullptr},
        {"IN_C", &m_val_c, 0, nullptr},
        {"IN_D", &m_val_d, 0, nullptr}
    };

    int err_pos;
    m_compiled_expr = te_compile(m_expression.c_str(), vars, 4, &err_pos);

    if (m_compiled_expr == nullptr) {
        ESP_LOGE(TAG, "[%s] Erro de sintaxe proximo ao caractere %d", m_id.c_str(), err_pos);
        return false;
    }

    ESP_LOGI(TAG, "[%s] Expressao pronta: %s", m_id.c_str(), m_expression.c_str());
    return true;
}

std::string MathNodeBlock::getId() const 
{ 
    return m_id; 
}

void* MathNodeBlock::getDataOutput(const std::string& port_name) 
{
    if (port_name == "OUT") {
        return &m_out;
    }
    return nullptr;
}

bool MathNodeBlock::connectDataInput(const std::string& port_name, void* data_pointer) 
{
    if (port_name == "IN_A") { m_in_a = static_cast<float*>(data_pointer); return true; }
    if (port_name == "IN_B") { m_in_b = static_cast<float*>(data_pointer); return true; }
    if (port_name == "IN_C") { m_in_c = static_cast<float*>(data_pointer); return true; }
    if (port_name == "IN_D") { m_in_d = static_cast<float*>(data_pointer); return true; }
    return false;
}

void MathNodeBlock::triggerEventInput(const std::string& event_name) 
{
    if (event_name == "REQ") {
        m_val_a = (m_in_a != nullptr) ? static_cast<double>(*m_in_a) : 0.0;
        m_val_b = (m_in_b != nullptr) ? static_cast<double>(*m_in_b) : 0.0;
        m_val_c = (m_in_c != nullptr) ? static_cast<double>(*m_in_c) : 0.0;
        m_val_d = (m_in_d != nullptr) ? static_cast<double>(*m_in_d) : 0.0;

        if (m_compiled_expr != nullptr) {
            m_out = static_cast<float>(te_eval(m_compiled_expr));
        }

        emitEvent("CNF");
    }
}

IFunctionBlock* MathNodeBlock::create(const std::string& block_id, cJSON* config) 
{
    std::string expr = "0"; 
    if (config != nullptr) {
        cJSON* e = cJSON_GetObjectItem(config, "expression");
        if (cJSON_IsString(e)) {
            expr = e->valuestring;
        }
    }
    return new MathNodeBlock(block_id, expr);
}

static bool registered = []() {
    BlockRegistry::registerBlock("MathNode", MathNodeBlock::create);
    return true;
}();

} /* namespace Cefet */