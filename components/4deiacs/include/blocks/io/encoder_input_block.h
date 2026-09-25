/**
 * @file encoder_input_block.h
 * @brief Definicao do bloco de entrada de encoder rotativo via PCNT.
 */
#pragma once

#include "i_function_block.h"
#include "cJSON.h"
#include "driver/pulse_cnt.h"
#include <string>

namespace Cefet {

/**
 * @class EncoderInputBlock
 * @brief Bloco que le o hardware PCNT para contar pulsos de um encoder.
 */
class EncoderInputBlock : public IFunctionBlock {
public:
    EncoderInputBlock(const std::string& block_id, int gpio_a, int gpio_b, int ppr, int high_limit, int low_limit);
    virtual ~EncoderInputBlock();

    bool initialize();
    std::string getId() const override;

    void* getDataOutput(const std::string& port_name) override;
    void triggerEventInput(const std::string& event_name) override;

    static IFunctionBlock* create(const std::string& block_id, cJSON* config);

private:
    std::string m_id;
    int m_gpio_a;
    int m_gpio_b;
    int m_ppr;
    int m_high_limit;
    int m_low_limit;

    pcnt_unit_handle_t m_pcnt_unit;
    pcnt_channel_handle_t m_pcnt_chan_a;
    pcnt_channel_handle_t m_pcnt_chan_b;
    bool m_initialized;

    float m_pulses_out;
    float m_angle_out;
};

} // namespace Cefet