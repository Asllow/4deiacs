/**
 * @file encoder_input_block.cpp
 * @brief Implementacao do bloco de encoder.
 */
#include "blocks/io/encoder_input_block.h"
#include "block_registry.h"
#include "esp_log.h"

namespace Cefet {

static const char* TAG = "ENCODER_INPUT_BLOCK";

EncoderInputBlock::EncoderInputBlock(const std::string& block_id, int gpio_a, int gpio_b, int ppr, int high_limit, int low_limit)
    : m_id(block_id), m_gpio_a(gpio_a), m_gpio_b(gpio_b), m_ppr(ppr), m_high_limit(high_limit), m_low_limit(low_limit),
      m_pcnt_unit(nullptr), m_pcnt_chan_a(nullptr), m_pcnt_chan_b(nullptr), m_initialized(false), m_pulses_out(0.0f), m_angle_out(0.0f)
{
}

EncoderInputBlock::~EncoderInputBlock()
{
    if (m_pcnt_unit != nullptr) {
        pcnt_unit_stop(m_pcnt_unit);
        pcnt_unit_disable(m_pcnt_unit);
        pcnt_del_channel(m_pcnt_chan_a);
        pcnt_del_channel(m_pcnt_chan_b);
        pcnt_del_unit(m_pcnt_unit);
    }
}

bool EncoderInputBlock::initialize()
{
    if (m_initialized) return true;

    pcnt_unit_config_t unit_config = {};
    unit_config.high_limit = m_high_limit;
    unit_config.low_limit = m_low_limit;
    
    if (pcnt_new_unit(&unit_config, &m_pcnt_unit) != ESP_OK) {
        ESP_LOGE(TAG, "[%s] Falha ao alocar unidade PCNT.", m_id.c_str());
        return false;
    }

    pcnt_glitch_filter_config_t filter_config = {};
    filter_config.max_glitch_ns = 1000;
    pcnt_unit_set_glitch_filter(m_pcnt_unit, &filter_config);

    pcnt_chan_config_t chan_a_config = {};
    chan_a_config.edge_gpio_num = m_gpio_a;
    chan_a_config.level_gpio_num = m_gpio_b;
    pcnt_new_channel(m_pcnt_unit, &chan_a_config, &m_pcnt_chan_a);

    pcnt_chan_config_t chan_b_config = {};
    chan_b_config.edge_gpio_num = m_gpio_b;
    chan_b_config.level_gpio_num = m_gpio_a;
    pcnt_new_channel(m_pcnt_unit, &chan_b_config, &m_pcnt_chan_b);

    pcnt_channel_set_edge_action(m_pcnt_chan_a, PCNT_CHANNEL_EDGE_ACTION_DECREASE, PCNT_CHANNEL_EDGE_ACTION_INCREASE);
    pcnt_channel_set_level_action(m_pcnt_chan_a, PCNT_CHANNEL_LEVEL_ACTION_KEEP, PCNT_CHANNEL_LEVEL_ACTION_INVERSE);
    
    pcnt_channel_set_edge_action(m_pcnt_chan_b, PCNT_CHANNEL_EDGE_ACTION_INCREASE, PCNT_CHANNEL_EDGE_ACTION_DECREASE);
    pcnt_channel_set_level_action(m_pcnt_chan_b, PCNT_CHANNEL_LEVEL_ACTION_KEEP, PCNT_CHANNEL_LEVEL_ACTION_INVERSE);

    pcnt_unit_enable(m_pcnt_unit);
    pcnt_unit_clear_count(m_pcnt_unit);
    pcnt_unit_start(m_pcnt_unit);

    m_initialized = true;
    ESP_LOGI(TAG, "[%s] Bloco Encoder inicializado com sucesso (A:%d, B:%d).", m_id.c_str(), m_gpio_a, m_gpio_b);
    return true;
}

std::string EncoderInputBlock::getId() const { return m_id; }

void* EncoderInputBlock::getDataOutput(const std::string& port_name)
{
    if (port_name == "PULSES") return &m_pulses_out;
    if (port_name == "ANGLE")  return &m_angle_out;
    return nullptr;
}

void EncoderInputBlock::triggerEventInput(const std::string& event_name)
{
    if (event_name == "REQ") {
        if (!m_initialized) initialize();

        int pulse_count = 0;
        if (pcnt_unit_get_count(m_pcnt_unit, &pulse_count) == ESP_OK) {
            m_pulses_out = static_cast<float>(pulse_count);
            m_angle_out = -(6.283185f * m_pulses_out) / static_cast<float>(m_ppr);
        }
        emitEvent("CNF");
    }
}

IFunctionBlock* EncoderInputBlock::create(const std::string& block_id, cJSON* config)
{
    int gpio_a = 16, gpio_b = 17, ppr = 10000, high_limit = 10000, low_limit = -10000;

    if (config != nullptr) {
        cJSON* item = nullptr;
        if ((item = cJSON_GetObjectItem(config, "gpio_a")) && cJSON_IsNumber(item)) gpio_a = item->valueint;
        if ((item = cJSON_GetObjectItem(config, "gpio_b")) && cJSON_IsNumber(item)) gpio_b = item->valueint;
        if ((item = cJSON_GetObjectItem(config, "ppr")) && cJSON_IsNumber(item)) ppr = item->valueint;
        if ((item = cJSON_GetObjectItem(config, "high_limit")) && cJSON_IsNumber(item)) high_limit = item->valueint;
        if ((item = cJSON_GetObjectItem(config, "low_limit")) && cJSON_IsNumber(item)) low_limit = item->valueint;
    }

    return new EncoderInputBlock(block_id, gpio_a, gpio_b, ppr, high_limit, low_limit);
}

/**
 * @brief Registo estatico na Factory.
 */
static bool registered = []() {
    BlockRegistry::registerBlock("EncoderInput", EncoderInputBlock::create);
    return true;
}();

} // namespace Cefet