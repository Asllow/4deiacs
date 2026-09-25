#include "e_cycle_block.h"
#include "4deiacs_node_engine.h"
#include "block_registry.h"
#include "esp_log.h"

namespace deiacs {

static const char* TAG = "E_CYCLE_BLOCK";

ECycleBlock::ECycleBlock(const std::string& block_id, uint64_t period_ms, EventIds target_event)
    : m_id(block_id), m_period_ms(period_ms), m_target_event(target_event), m_timer_handle(nullptr)
{
}

ECycleBlock::~ECycleBlock()
{
    if (m_timer_handle != nullptr) {
        stopTimer();
        esp_timer_delete(m_timer_handle);
    }
}

bool ECycleBlock::initialize()
{
    esp_timer_create_args_t timer_args = {};
    timer_args.callback = &ECycleBlock::timerCallback;
    timer_args.arg = this;
    timer_args.name = m_id.c_str();

    esp_err_t err = esp_timer_create(&timer_args, &m_timer_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "[%s] Failed to create high-resolution timer.", m_id.c_str());
        return false;
    }

    ESP_LOGI(TAG, "[%s] Timer initialized (Period: %llu ms).", m_id.c_str(), m_period_ms);
    startTimer();
    return true;
}

std::string ECycleBlock::getId() const
{
    return m_id;
}

void ECycleBlock::startTimer()
{
    if (m_timer_handle != nullptr) {
        esp_timer_start_periodic(m_timer_handle, m_period_ms * 1000);
        ESP_LOGI(TAG, "[%s] Timer started.", m_id.c_str());
    }
}

void ECycleBlock::stopTimer()
{
    if (m_timer_handle != nullptr) {
        esp_timer_stop(m_timer_handle);
        ESP_LOGI(TAG, "[%s] Timer stopped.", m_id.c_str());
    }
}

void ECycleBlock::timerCallback(void* arg)
{
    ECycleBlock* instance = static_cast<ECycleBlock*>(arg);

    instance->emitEvent("EV_OUT");
}

IFunctionBlock* ECycleBlock::create(const std::string& block_id, cJSON* config)
{
    uint64_t period = 1000;
    int event_id = 1;

    if (config != nullptr) {
        cJSON* period_item = cJSON_GetObjectItem(config, "period_ms");
        if (cJSON_IsNumber(period_item)) {
            period = static_cast<uint64_t>(period_item->valuedouble);
        }

        cJSON* event_item = cJSON_GetObjectItem(config, "target_event");
        if (cJSON_IsNumber(event_item)) {
            event_id = event_item->valueint;
        }
    }

    return new ECycleBlock(block_id, period, static_cast<EventIds>(event_id));
}

/**
 * @brief Static block registration.
 * Executes prior to app_main to register the factory method into the BlockRegistry.
 */
static bool registered = []() {
    BlockRegistry::registerBlock("ECycle", ECycleBlock::create);
    return true;
}();

} // namespace deiacs