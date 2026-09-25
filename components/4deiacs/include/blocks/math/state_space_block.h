/**
 * @file state_space_block.h
 * @brief Multivariable State Space (LQR) Controller Block.
 */
#pragma once

#include <string>
#include <vector>
#include "i_function_block.h"
#include "cJSON.h"

namespace deiacs {

/**
 * @brief State Space Control Service Interface Function Block (SIFB).
 *
 * Calculates the control law U = -K * X in real-time.
 * The gain matrix K is defined in the JSON manifest, allowing
 * the block to dynamically adapt to SISO (1x1) or MIMO (MxN) systems
 * without the need to recompile the firmware.
 * * Dynamically generated ports:
 * - Inputs: X_0, X_1, ..., X_n (Pointers to current states)
 * - Outputs: U_0, U_1, ..., U_m (Calculated actuation signals)
 */
class StateSpaceBlock : public IFunctionBlock {
public:
    /**
     * @brief Constructor with dynamic matrix allocation.
     * @param block_id Unique block identifier.
     * @param k_matrix Gain matrix K [rows(U) x cols(X)].
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
    
    /* Controller gain matrix (K) */
    std::vector<std::vector<float>> m_K;
    
    /* Dimensions automatically discovered from JSON */
    size_t m_num_states;  /* Number of columns in K (Vector X) */
    size_t m_num_outputs; /* Number of rows in K (Vector U) */

    /* Dynamic float typed I/O arrays (Solid / Polymorphism) */
    std::vector<float*> m_x_in; 
    std::vector<float> m_u_out; 
};

} /* namespace deiacs */