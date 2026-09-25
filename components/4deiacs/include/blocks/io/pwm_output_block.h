/**
 * @file pwm_output_block.h
 * @brief Hardware Interface Function Block for PWM generation.
 * Follows SOLID principles, acting decoupled from the event engine.
 */
#pragma once

#include <string>
#include <vector>
#include "driver/ledc.h"
#include "i_function_block.h"
#include "cJSON.h"

namespace deiacs {

/**
 * @class PwmOutputBlock
 * @brief Encapsulates the ESP-IDF LEDC peripheral for standard PWM signal generation.
 */
class PwmOutputBlock : public IFunctionBlock {
public:
    /**
     * @brief Default constructor.
     * 
     * @param block_id Unique identifier in the IEC 61499 network.
     * @param gpio_num Hardware output pin.
     * @param timer_num Associated LEDC timer instance.
     * @param channel_num Allocated LEDC channel.
     * @param freq_hz Operating frequency in Hertz.
     */
    PwmOutputBlock(const std::string& block_id, int gpio_num, ledc_timer_t timer_num, ledc_channel_t channel_num, uint32_t freq_hz);

    /**
     * @brief Default destructor that safely stops the hardware.
     */
    ~PwmOutputBlock() override;

    bool initialize() override;
    std::string getId() const override;
    
    /**
     * @brief Connects an external memory pointer to a generic data input port.
     * 
     * @param port_name Port name (e.g., "DUTY_CYCLE").
     * @param data_pointer Pointer to the variable in heap memory.
     * @return true if the port exists and is successfully coupled.
     */
    bool connectDataInput(const std::string& port_name, void* data_pointer) override;

    /**
     * @brief Polymorphic trigger for event-based execution.
     * 
     * @param event_name Input event name (e.g., "REQ").
     */
    void triggerEventInput(const std::string& event_name) override;

    static IFunctionBlock* create(const std::string& block_id, cJSON* config);

private:
    std::string m_id;
    int m_gpio_num;
    ledc_timer_t m_timer_num;
    ledc_channel_t m_channel_num;
    uint32_t m_freq_hz;
    std::vector<float*> m_inputs;

    /**
     * @brief Low-level modification of the PWM duty cycle.
     * 
     * @param duty_cycle Raw duty cycle value (0-8191 for 13-bit).
     * @return true on bus success.
     */
    bool writePwm(uint32_t duty_cycle);
};

} // namespace deiacs