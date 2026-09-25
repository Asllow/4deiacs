/**
 * @file state_space_block.cpp
 * @brief Implementacao da algebra linear do Espaco de Estados.
 */
#include "state_space_block.h"
#include "block_registry.h"
#include "esp_log.h"

namespace Cefet {

static const char* TAG = "STATE_SPACE";

StateSpaceBlock::StateSpaceBlock(const std::string& block_id, const std::vector<std::vector<float>>& k_matrix)
    : m_id(block_id), m_K(k_matrix), m_num_states(0), m_num_outputs(0)
{
    m_num_outputs = m_K.size();
    if (m_num_outputs > 0) {
        m_num_states = m_K[0].size();
    }

    /* Redimensionamento seguro das portas baseando-se na matriz de ganhos */
    m_x_in.resize(m_num_states, nullptr);
    m_u_out.resize(m_num_outputs, 0.0f);
}

StateSpaceBlock::~StateSpaceBlock()
{
    /* std::vector limpa a sua propria memoria automaticamente */
}

bool StateSpaceBlock::initialize()
{
    if (m_num_outputs == 0 || m_num_states == 0) {
        ESP_LOGE(TAG, "[%s] Matriz K invalida ou vazia.", m_id.c_str());
        return false;
    }

    ESP_LOGI(TAG, "[%s] Inicializado. Entradas(X): %d | Saidas(U): %d", 
             m_id.c_str(), m_num_states, m_num_outputs);
    return true;
}

std::string StateSpaceBlock::getId() const
{
    return m_id;
}

/* ========================================================================= */
/* ALOCACAO DINAMICA DE PORTAS                                               */
/* ========================================================================= */

void* StateSpaceBlock::getDataOutput(const std::string& port_name)
{
    /* Permite ligacoes como U_0, U_1, etc. */
    if (port_name.find("U_") == 0) {
        size_t idx = std::stoi(port_name.substr(2));
        if (idx < m_num_outputs) {
            return &m_u_out[idx];
        }
    }
    return nullptr;
}

bool StateSpaceBlock::connectDataInput(const std::string& port_name, void* data_pointer)
{
    /* Permite ligacoes como X_0, X_1, etc. */
    if (port_name.find("X_") == 0) {
        size_t idx = std::stoi(port_name.substr(2));
        if (idx < m_num_states) {
            m_x_in[idx] = static_cast<float*>(data_pointer);
            return true;
        }
    }
    return false;
}

/* ========================================================================= */
/* NUCLEO MATEMATICO (U = -K * X)                                            */
/* ========================================================================= */

void StateSpaceBlock::triggerEventInput(const std::string& event_name)
{
    if (event_name == "REQ") {
        
        /* Multiplicacao de matrizes otimizada (Linha x Coluna) */
        for (size_t i = 0; i < m_num_outputs; ++i) {
            float sum = 0.0f;
            
            for (size_t j = 0; j < m_num_states; ++j) {
                /* Confirma se o ponteiro da malha foi devidamente ligado no JSON */
                float x_val = (m_x_in[j] != nullptr) ? *(m_x_in[j]) : 0.0f;
                sum += m_K[i][j] * x_val;
            }
            
            /* Realimentacao de estados: U = -KX */
            m_u_out[i] = -sum;
        }

        emitEvent("CNF");
    }
}

/* ========================================================================= */
/* FACTORY & DESERIALIZACAO JSON                                             */
/* ========================================================================= */

IFunctionBlock* StateSpaceBlock::create(const std::string& block_id, cJSON* config)
{
    std::vector<std::vector<float>> k_matrix;

    if (config != nullptr) {
        cJSON* k_array = cJSON_GetObjectItem(config, "K");
        
        if (k_array != nullptr && cJSON_IsArray(k_array)) {
            int rows = cJSON_GetArraySize(k_array);
            
            for (int i = 0; i < rows; ++i) {
                cJSON* row_item = cJSON_GetArrayItem(k_array, i);
                
                if (cJSON_IsArray(row_item)) {
                    int cols = cJSON_GetArraySize(row_item);
                    std::vector<float> current_row;
                    
                    for (int j = 0; j < cols; ++j) {
                        cJSON* val_item = cJSON_GetArrayItem(row_item, j);
                        if (cJSON_IsNumber(val_item)) {
                            current_row.push_back(static_cast<float>(val_item->valuedouble));
                        }
                    }
                    k_matrix.push_back(current_row);
                }
            }
        }
    }

    /* Protecao contra falhas no JSON: Fallback para sistema nulo 1x1 */
    if (k_matrix.empty()) {
        k_matrix.push_back(std::vector<float>(1, 0.0f));
    }

    return new StateSpaceBlock(block_id, k_matrix);
}

static bool registered = []() {
    BlockRegistry::registerBlock("StateSpace", StateSpaceBlock::create);
    return true;
}();

} /* namespace Cefet */