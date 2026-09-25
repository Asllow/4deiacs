/**
 * @file pwm_output_block.cpp
 * @brief Implementacao do bloco PWM Output aderente a norma IEC 61499.
 */
#include "blocks/io/pwm_output_block.h"
#include "block_registry.h"
#include "esp_log.h"

namespace Cefet {

static const char* TAG = "PWM_OUTPUT_BLOCK";

PwmOutputBlock::PwmOutputBlock(const std::string& block_id, int gpio_num, ledc_timer_t timer_num, ledc_channel_t channel_num, uint32_t freq_hz)
    : m_id(block_id), m_gpio_num(gpio_num), m_timer_num(timer_num), m_channel_num(channel_num), m_freq_hz(freq_hz)
{
    /* Aloca espaco generico para a porta de entrada (DUTY_CYCLE) */
    m_inputs.resize(1, nullptr);
}

PwmOutputBlock::~PwmOutputBlock()
{
    ledc_stop(LEDC_LOW_SPEED_MODE, m_channel_num, 0);
}

bool PwmOutputBlock::initialize()
{
    ledc_timer_config_t ledc_timer = {};
    ledc_timer.speed_mode       = LEDC_LOW_SPEED_MODE;
    ledc_timer.timer_num        = m_timer_num;
    ledc_timer.duty_resolution  = LEDC_TIMER_12_BIT;
    ledc_timer.freq_hz          = m_freq_hz;
    ledc_timer.clk_cfg          = LEDC_AUTO_CLK;

    if (ledc_timer_config(&ledc_timer) != ESP_OK) {
        ESP_LOGE(TAG, "[%s] Failed to configure PWM timer.", m_id.c_str());
        return false;
    }

    ledc_channel_config_t ledc_channel = {};
    ledc_channel.speed_mode     = LEDC_LOW_SPEED_MODE;
    ledc_channel.channel        = m_channel_num;
    ledc_channel.timer_sel      = m_timer_num;
    ledc_channel.gpio_num       = m_gpio_num;
    ledc_channel.duty           = 0;
    ledc_channel.hpoint         = 0;

    if (ledc_channel_config(&ledc_channel) != ESP_OK) {
        ESP_LOGE(TAG, "[%s] Failed to configure PWM channel.", m_id.c_str());
        return false;
    }

    ESP_LOGI(TAG, "[%s] PWM Block initialized. Pin: %d, Freq: %lu Hz", m_id.c_str(), m_gpio_num, m_freq_hz);
    return true;
}

std::string PwmOutputBlock::getId() const
{
    return m_id;
}

bool PwmOutputBlock::connectDataInput(const std::string& port_name, void* data_pointer)
{
    if (port_name == "DUTY_CYCLE") {
        m_inputs[0] = static_cast<float*>(data_pointer);
        return true;
    }
    return false;
}

void PwmOutputBlock::triggerEventInput(const std::string& event_name)
{
    if (event_name == "REQ") {
        if (m_inputs[0] != nullptr) {
            float duty_percent = *(m_inputs[0]);
            
            /* Saturacao de seguranca */
            if (duty_percent < 0.0f) duty_percent = 0.0f;
            if (duty_percent > 100.0f) duty_percent = 100.0f;

            /* Conversao de Porcentagem (0-100) para Resolucao de 12 bits (0-4095) */
            uint32_t duty_raw = static_cast<uint32_t>((duty_percent / 100.0f) * 4095.0f);
            writePwm(duty_raw);
        }
        emitEvent("CNF");
    }
}

bool PwmOutputBlock::writePwm(uint32_t duty_cycle)
{
    if (ledc_set_duty(LEDC_LOW_SPEED_MODE, m_channel_num, duty_cycle) != ESP_OK) {
        return false;
    }
    if (ledc_update_duty(LEDC_LOW_SPEED_MODE, m_channel_num) != ESP_OK) {
        return false;
    }
    return true;
}

IFunctionBlock* PwmOutputBlock::create(const std::string& block_id, cJSON* config)
{
    int gpio = 2;               
    int timer = 0;              
    int channel = 0;            
    uint32_t freq = 1000;       

    if (config != nullptr) {
        cJSON* gpio_item = cJSON_GetObjectItem(config, "gpio");
        if (cJSON_IsNumber(gpio_item)) gpio = gpio_item->valueint;

        cJSON* timer_item = cJSON_GetObjectItem(config, "timer");
        if (cJSON_IsNumber(timer_item)) timer = timer_item->valueint;

        cJSON* channel_item = cJSON_GetObjectItem(config, "channel");
        if (cJSON_IsNumber(channel_item)) channel = channel_item->valueint;

        cJSON* freq_item = cJSON_GetObjectItem(config, "freq");
        if (cJSON_IsNumber(freq_item)) freq = static_cast<uint32_t>(freq_item->valueint);
    }

    return new PwmOutputBlock(block_id, gpio, static_cast<ledc_timer_t>(timer), static_cast<ledc_channel_t>(channel), freq);
}

static bool registered = []() {
    BlockRegistry::registerBlock("PwmOutput", PwmOutputBlock::create);
    return true;
}();

} // namespace Cefet