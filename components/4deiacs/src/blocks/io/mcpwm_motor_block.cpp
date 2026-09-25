#include "mcpwm_motor_block.h"
#include "block_registry.h"
#include "esp_log.h"
#include <algorithm>
#include <cmath>

namespace Cefet {

static const char* TAG = "MCPWM_MOTOR_BLOCK";

/**
 * @brief Internal MCPWM timer resolution in Hertz.
 * Defines the granularity of the Duty Cycle calculation.
 */
constexpr uint32_t MCPWM_TIMER_RESOLUTION_HZ = 10000000; 

McpwmMotorBlock::McpwmMotorBlock(const std::string& block_id, int gpio_pwm_a, int gpio_pwm_b, uint32_t freq_hz)
    : m_id(block_id), m_gpio_a(gpio_pwm_a), m_gpio_b(gpio_pwm_b), m_freq_hz(freq_hz),
      m_timer(nullptr), m_oper(nullptr), m_cmpr(nullptr), m_gen_a(nullptr), m_gen_b(nullptr)
{
    m_period_ticks = MCPWM_TIMER_RESOLUTION_HZ / m_freq_hz;
}

McpwmMotorBlock::~McpwmMotorBlock()
{
    if (m_gen_a) mcpwm_del_generator(m_gen_a);
    if (m_gen_b) mcpwm_del_generator(m_gen_b);
    if (m_cmpr) mcpwm_del_comparator(m_cmpr);
    if (m_oper) mcpwm_del_operator(m_oper);
    if (m_timer) mcpwm_del_timer(m_timer);
}

bool McpwmMotorBlock::initialize()
{
    mcpwm_timer_config_t timer_config = {};
    timer_config.group_id = 0;
    timer_config.clk_src = MCPWM_TIMER_CLK_SRC_DEFAULT;
    timer_config.resolution_hz = MCPWM_TIMER_RESOLUTION_HZ;
    timer_config.period_ticks = m_period_ticks;
    timer_config.count_mode = MCPWM_TIMER_COUNT_MODE_UP;

    if (mcpwm_new_timer(&timer_config, &m_timer) != ESP_OK) return false;

    mcpwm_operator_config_t oper_config = {};
    oper_config.group_id = 0;
    if (mcpwm_new_operator(&oper_config, &m_oper) != ESP_OK) return false;
    if (mcpwm_operator_connect_timer(m_oper, m_timer) != ESP_OK) return false;

    mcpwm_comparator_config_t cmpr_config = {};
    cmpr_config.flags.update_cmp_on_tez = true;
    if (mcpwm_new_comparator(m_oper, &cmpr_config, &m_cmpr) != ESP_OK) return false;

    mcpwm_generator_config_t gen_a_config = {};
    gen_a_config.gen_gpio_num = m_gpio_a;
    if (mcpwm_new_generator(m_oper, &gen_a_config, &m_gen_a) != ESP_OK) return false;

    mcpwm_generator_config_t gen_b_config = {};
    gen_b_config.gen_gpio_num = m_gpio_b;
    if (mcpwm_new_generator(m_oper, &gen_b_config, &m_gen_b) != ESP_OK) return false;

    mcpwm_comparator_set_compare_value(m_cmpr, 0);

    mcpwm_generator_set_action_on_timer_event(m_gen_a, MCPWM_GEN_TIMER_EVENT_ACTION(MCPWM_TIMER_DIRECTION_UP, MCPWM_TIMER_EVENT_EMPTY, MCPWM_GEN_ACTION_HIGH));
    mcpwm_generator_set_action_on_compare_event(m_gen_a, MCPWM_GEN_COMPARE_EVENT_ACTION(MCPWM_TIMER_DIRECTION_UP, m_cmpr, MCPWM_GEN_ACTION_LOW));
    
    mcpwm_generator_set_action_on_timer_event(m_gen_b, MCPWM_GEN_TIMER_EVENT_ACTION(MCPWM_TIMER_DIRECTION_UP, MCPWM_TIMER_EVENT_EMPTY, MCPWM_GEN_ACTION_HIGH));
    mcpwm_generator_set_action_on_compare_event(m_gen_b, MCPWM_GEN_COMPARE_EVENT_ACTION(MCPWM_TIMER_DIRECTION_UP, m_cmpr, MCPWM_GEN_ACTION_LOW));

    mcpwm_generator_set_force_level(m_gen_a, -1, true);
    mcpwm_generator_set_force_level(m_gen_b, -1, true);

    if (mcpwm_timer_enable(m_timer) != ESP_OK) return false;
    if (mcpwm_timer_start_stop(m_timer, MCPWM_TIMER_START_NO_STOP) != ESP_OK) return false;

    ESP_LOGI(TAG, "[%s] MCPWM Motor Block initialized (Pins: %d, %d | Freq: %lu Hz).", m_id.c_str(), m_gpio_a, m_gpio_b, m_freq_hz);
    return true;
}

std::string McpwmMotorBlock::getId() const
{
    return m_id;
}

bool McpwmMotorBlock::setSpeed(float speed_percent)
{
    if (!m_cmpr || !m_gen_a || !m_gen_b) return false;

    speed_percent = std::clamp(speed_percent, -100.0f, 100.0f);
    
    uint32_t duty_ticks = static_cast<uint32_t>((std::abs(speed_percent) / 100.0f) * m_period_ticks);
    mcpwm_comparator_set_compare_value(m_cmpr, duty_ticks);

    if (speed_percent > 0.0f) {
        mcpwm_generator_set_force_level(m_gen_a, -1, false);
        mcpwm_generator_set_force_level(m_gen_b, 0, true);
    } 
    else if (speed_percent < 0.0f) {
        mcpwm_generator_set_force_level(m_gen_a, 0, true);
        mcpwm_generator_set_force_level(m_gen_b, -1, false);
    } 
    else {
        mcpwm_generator_set_force_level(m_gen_a, 0, true);
        mcpwm_generator_set_force_level(m_gen_b, 0, true);
    }

    return true;
}

IFunctionBlock* McpwmMotorBlock::create(const std::string& block_id, cJSON* config)
{
    int gpio_a = 15;            // Default GPIO IN1
    int gpio_b = 16;            // Default GPIO IN2
    uint32_t freq = 20000;      // Default Frequency (Hz)

    if (config != nullptr) {
        cJSON* gpio_a_item = cJSON_GetObjectItem(config, "gpio_a");
        if (cJSON_IsNumber(gpio_a_item)) {
            gpio_a = gpio_a_item->valueint;
        }

        cJSON* gpio_b_item = cJSON_GetObjectItem(config, "gpio_b");
        if (cJSON_IsNumber(gpio_b_item)) {
            gpio_b = gpio_b_item->valueint;
        }

        cJSON* freq_item = cJSON_GetObjectItem(config, "freq");
        if (cJSON_IsNumber(freq_item)) {
            freq = static_cast<uint32_t>(freq_item->valueint);
        }
    }

    return new McpwmMotorBlock(block_id, gpio_a, gpio_b, freq);
}

/**
 * @brief Static block registration.
 * Executes prior to app_main to register the factory method into the BlockRegistry.
 */
static bool registered = []() {
    BlockRegistry::registerBlock("McpwmMotor", McpwmMotorBlock::create);
    return true;
}();

} // namespace Cefet