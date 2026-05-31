#include <stdio.h>
#include <string.h>

#include "driver/gpio.h"
#include "driver/touch_pad.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "sdkconfig.h"
#include "soc/touch_sensor_channel.h"

#define TOUCH_PAD_NUM TOUCH_PAD_NUM11_GPIO_NUM
#define BASELINE_SAMPLES 32
#define ACTIVE_DELTA 10000
#define EMA_ALPHA 0.3f

#define LED_GPIO 7

const static char* TAG = "[Entrypoint]";

bool led_active = false;

typedef struct {
    touch_pad_t pad;
    const char* name;
    uint32_t baseline;
    float ema;
} fruit_entry_t;

esp_err_t eLedInit(void) {
    gpio_reset_pin(LED_GPIO);
    gpio_set_direction(LED_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_level(LED_GPIO, 0);
    return ESP_OK;
}

void app_main(void) {
    eLedInit();
    uint32_t touch_value;

    fruit_entry_t fruit = {
        .pad = TOUCH_PAD_NUM,
        .name = "banana",
        .baseline = 0,
        .ema = 0,
    };

    esp_err_t res = touch_pad_init();
    if (res != ESP_OK) {
        ESP_LOGD(TAG, "Failed to init Touch Pad Driver");
    }

    touch_pad_config(TOUCH_PAD_NUM);

    touch_pad_denoise_t denoise = {
        .grade = TOUCH_PAD_DENOISE_BIT4,
        .cap_level = TOUCH_PAD_DENOISE_CAP_L4,
    };
    touch_pad_denoise_set_config(&denoise);
    touch_pad_denoise_enable();
    ESP_LOGI(TAG, "Denoise function configured");

    touch_pad_set_fsm_mode(TOUCH_FSM_MODE_TIMER);
    touch_pad_fsm_start();

    vTaskDelay(pdMS_TO_TICKS(200));

    uint32_t sum = 0;
    for (uint8_t i = 0; i < BASELINE_SAMPLES; i++) {
        touch_pad_read_raw_data(TOUCH_PAD_NUM, &touch_value);
        sum += touch_value;
        vTaskDelay(pdMS_TO_TICKS(20));
    }
    fruit.baseline = sum / BASELINE_SAMPLES;
    fruit.ema = (float)fruit.baseline;
    ESP_LOGI(TAG, "Baseline: %4" PRIu32 ". EMA: %.2f", fruit.baseline, fruit.ema);

    while (1) {
        touch_pad_read_raw_data(TOUCH_PAD_NUM, &touch_value);

        fruit.ema = (fruit.ema * (1.0f - EMA_ALPHA)) + (touch_value * EMA_ALPHA);
        ESP_LOGI(TAG, "T%d: [%4" PRIu32 "] %.2f", TOUCH_PAD_NUM, touch_value, fruit.ema);
        if ((fruit.ema - fruit.baseline) > ACTIVE_DELTA) {
            if (led_active == false) {
                led_active = true;
                gpio_set_level(LED_GPIO, 1);
            }
            ESP_LOGI(TAG, "Touch found!");
        } else {
            if (led_active == true) {
                led_active = false;
                gpio_set_level(LED_GPIO, 0);
            }
        }
        vTaskDelay(pdMS_TO_TICKS(20));
    }

    res = touch_pad_deinit();
    if (res != ESP_OK) {
        ESP_LOGD(TAG, "Failed to de-init Touch Pad Driver");
    }
}
