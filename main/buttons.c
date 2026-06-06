#include "buttons.h"

#include "driver/gpio.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/timers.h"

static const char* TAG = "[BUTTONS]";

static Button_t buttons[BUTTONS_MAX_AMOUNT];
static uint8_t amount = 0;

static void vHandleDebounceTimer(TimerHandle_t xTimer);
static void vIsrHandler(void* arg);

esp_err_t xButtonConfigure(ButtonCfg_t* cfg) {
    if (amount >= BUTTONS_MAX_AMOUNT) {
        ESP_LOGE(TAG, "Maximum number of buttons reached. Cannot configure more.");
        return ESP_ERR_NOT_ALLOWED;
    }

    ESP_LOGI(TAG, "Configuring button %u with gpio %u", amount, cfg->gpio);
    buttons[amount].cfg.gpio = cfg->gpio;
    buttons[amount].cfg.fnCallback = cfg->fnCallback;
    buttons[amount].xDebounceTimer = xTimerCreate("Button Debounce Timers", pdMS_TO_TICKS(BUTTON_DEBOUNCE_TIME_MS), pdFALSE,
                                                  (void*)&buttons[amount], vHandleDebounceTimer);

    gpio_config_t btn_io_cfg = {
        .intr_type = GPIO_INTR_POSEDGE,
        .mode = GPIO_MODE_INPUT,
        .pin_bit_mask = (1ULL << cfg->gpio),
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .pull_up_en = GPIO_PULLUP_DISABLE,
    };
    ESP_ERROR_CHECK(gpio_config(&btn_io_cfg));
    ESP_ERROR_CHECK(gpio_isr_handler_add(cfg->gpio, vIsrHandler, (void*)&buttons[amount]));

    amount++;
    return ESP_OK;
}

static void vHandleDebounceTimer(TimerHandle_t xTimer) {
    Button_t* btn = (Button_t*)pvTimerGetTimerID(xTimer);
    if (gpio_get_level(btn->cfg.gpio) == 1) {
        ESP_LOGI(TAG, "Button %u pressed!", btn->cfg.gpio);
        if (btn->cfg.fnCallback != NULL) {
            btn->cfg.fnCallback(btn);
        }
    };
}

static void vIsrHandler(void* arg) {
    Button_t* btn = (Button_t*)arg;
    xTimerResetFromISR(btn->xDebounceTimer, NULL);
}
