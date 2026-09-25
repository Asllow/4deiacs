#pragma once

#include <string>
#include "esp_timer.h"
#include "i_function_block.h"
#include "cefet_events.h"
#include "cJSON.h"

namespace Cefet {

/**
 * @brief Cyclic Event Generator Service Interface Function Block (E_CYCLE).
 *
 * Utilizes the ESP-IDF high-resolution timer to generate periodic events
 * on the central bus. Essential for initiating execution chains, such as
 * periodic sensor sampling or PID control loop calculations.
 */
class ECycleBlock : public IFunctionBlock {
public:
    /**
     * @brief Instantiates the Cyclic Event Generator.
     *
     * @param block_id Unique network identifier for the block instance.
     * @param period_ms Event trigger period in milliseconds.
     * @param target_event Internal system event ID to be published per cycle.
     */
    ECycleBlock(const std::string& block_id, uint64_t period_ms, EventIds target_event);

    /**
     * @brief Destroys the block, stops the hardware timer and frees resources.
     */
    ~ECycleBlock() override;

    /**
     * @brief Initializes and allocates the hardware timer resources.
     *
     * @return true if successfully allocated.
     * @return false if system resources are exhausted.
     */
    bool initialize() override;

    /**
     * @brief Retrieves the block's unique identifier.
     *
     * @return std::string The configured block ID.
     */
    std::string getId() const override;

    /**
     * @brief Starts the hardware timer execution.
     */
    void startTimer();

    /**
     * @brief Suspends the hardware timer execution.
     */
    void stopTimer();

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
    uint64_t m_period_ms;
    EventIds m_target_event;
    esp_timer_handle_t m_timer_handle;

    /**
     * @brief Required static callback for the ESP-IDF esp_timer API.
     *
     * @param arg Opaque pointer (void*) containing the class instance (this).
     */
    static void timerCallback(void* arg);
};

} // namespace Cefet