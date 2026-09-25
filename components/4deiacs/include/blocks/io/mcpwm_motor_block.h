#pragma once

#include <string>
#include "i_function_block.h"
#include "driver/mcpwm_prelude.h"
#include "cJSON.h"

namespace Cefet {

/**
 * @brief DC Motor Control Service Interface Function Block (SIFB).
 *
 * Encapsulates the modern MCPWM API for H-Bridge driving.
 * Automatically manages complementary direction signals and pulse width,
 * isolating control logic from ESP-IDF v6.0 hardware allocation complexity.
 */
class McpwmMotorBlock : public IFunctionBlock {
public:
    /**
     * @brief Instantiates the MCPWM Motor Block.
     *
     * @param block_id Unique network identifier for the block instance.
     * @param gpio_pwm_a GPIO pin connected to IN1/PWM_R input of the H-Bridge.
     * @param gpio_pwm_b GPIO pin connected to IN2/PWM_L input of the H-Bridge.
     * @param freq_hz Switching frequency in Hertz (e.g., 20000 for 20kHz).
     */
    McpwmMotorBlock(const std::string& block_id, int gpio_pwm_a, int gpio_pwm_b, uint32_t freq_hz);

    /**
     * @brief Destroys the block and safely releases MCPWM hardware generators.
     */
    ~McpwmMotorBlock() override;

    /**
     * @brief Initializes the MCPWM pipeline (Timer, Operator, Comparator, Generators).
     *
     * @return true if all hardware modules are successfully allocated.
     */
    bool initialize() override;

    /**
     * @brief Retrieves the block's unique identifier.
     *
     * @return std::string The configured block ID.
     */
    std::string getId() const override;

    /**
     * @brief Updates DC motor speed and direction.
     *
     * @param speed_percent Percentage value (-100.0 to 100.0).
     * Positive values drive in one direction, negative in the opposite.
     * A value of 0.0 forces both outputs to freewheel/brake state.
     * @return true if the hardware accepted the command.
     */
    bool setSpeed(float speed_percent);

    /**
     * @brief Factory method for dynamic instantiation via JSON manifest.
     *
     * @param block_id Unique identifier for the new instance.
     * @param config cJSON pointer containing block-specific parameters.
     * @return IFunctionBlock* Pointer to the newly allocated instance.
     */
    static IFunctionBlock* create(const std::string& block_id, cJSON* config);

private:
    std::string m_id;
    int m_gpio_a;
    int m_gpio_b;
    uint32_t m_freq_hz;
    uint32_t m_period_ticks;

    mcpwm_timer_handle_t m_timer;
    mcpwm_oper_handle_t m_oper;
    mcpwm_cmpr_handle_t m_cmpr;
    mcpwm_gen_handle_t m_gen_a;
    mcpwm_gen_handle_t m_gen_b;
};

} // namespace Cefet