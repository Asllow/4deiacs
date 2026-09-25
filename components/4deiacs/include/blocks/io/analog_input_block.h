/**
 * @file analog_input_block.h
 * @brief Analog Input Service Interface (CSIFB) with standardized typing.
 */
#pragma once

#include <string>
#include "i_function_block.h"
#include "esp_adc/adc_oneshot.h"
#include "hal/adc_types.h"
#include "cJSON.h"

namespace deiacs {

/**
 * @brief Analog Input Service Interface Function Block (SIFB).
 *
 * Encapsulates the ESP-IDF v6.0 ADC driver.
 * Converts the raw read value to float, ensuring polymorphism
 * and pointer integrity when exchanging data with other mathematical 
 * and network blocks.
 */
class AnalogInputBlock : public IFunctionBlock {
public:
    /**
     * @brief Instantiates the Analog Input Block.
     *
     * @param block_id Unique network identifier.
     * @param adc_unit ADC hardware unit.
     * @param adc_channel ADC channel corresponding to the physical GPIO.
     */
    AnalogInputBlock(const std::string& block_id, adc_unit_t adc_unit, adc_channel_t adc_channel);

    /**
     * @brief Destructor. Frees ADC hardware resources.
     */
    ~AnalogInputBlock() override;

    /**
     * @brief Initializes and calibrates the ESP32 peripheral.
     *
     * @return true if allocation is successful.
     */
    bool initialize() override;

    /**
     * @brief Retrieves the block identification.
     *
     * @return std::string The configured ID.
     */
    std::string getId() const override;

    /**
     * @brief Performs raw analog-to-digital conversion.
     *
     * @param out_value Pointer to store the integer hardware result.
     * @return true if the read is successful.
     */
    bool readRaw(int* out_value);

    /* IEC 61499 PORTS */

    /**
     * @brief Exposes the memory address of the float-typed variable.
     */
    void* getDataOutput(const std::string& port_name) override;

    /**
     * @brief Processes input events (e.g., REQ).
     */
    void triggerEventInput(const std::string& event_name) override;

    /**
     * @brief Factory method for instantiation via JSON.
     */
    static IFunctionBlock* create(const std::string& block_id, cJSON* config);

private:
    std::string m_id;
    adc_unit_t m_unit;
    adc_channel_t m_channel;
    adc_oneshot_unit_handle_t m_adc_handle;
    bool m_initialized;

    /** 
     * @brief Internal variable typed as float for pointer safety.
     * Replaces the original int which caused memory corruption when 
     * read by mathematical blocks.
     */
    float m_data_out; 
};

} /* namespace deiacs */