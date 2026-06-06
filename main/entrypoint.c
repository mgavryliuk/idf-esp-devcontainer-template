#include <stdio.h>
#include <string.h>

#include "device.h"
#include "driver/gpio.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#include "sdkconfig.h"

static void vButtonISRHandler(void* arg);
static void vConfigure(void);
static void vHandleDebounceTimer(TimerHandle_t xTimer);

const static char* TAG = "[Entrypoint]";
volatile static bool isMotorRunning = false;
static TimerHandle_t xDebounceTimer;

void app_main(void) {
    vConfigure();
    while (1) {
        ESP_LOGD(TAG, "Cycle...");
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}

static void vConfigure(void) {
    ESP_LOGD(TAG, "Configure Motor's PIN %d", MOTOR_CONTROL_GPIO);
    gpio_config_t motor_gpio_cfg = {
        .intr_type = GPIO_INTR_DISABLE,
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .pin_bit_mask = MOTOR_CONTROL_GPIO_MASK,
    };
    gpio_config(&motor_gpio_cfg);

    ESP_LOGD(TAG, "Configure Button's PIN %d", BUTTON_GPIO);
    gpio_config_t btn_gpio_cfg = {
        .intr_type = GPIO_INTR_POSEDGE,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .pin_bit_mask = BUTTON_GPIO_MASK,
    };
    gpio_config(&btn_gpio_cfg);

    xDebounceTimer = xTimerCreate("Button debounce timer", pdMS_TO_TICKS(BUTTON_DEBOUNCE_TIME_MS), pdFALSE, NULL, vHandleDebounceTimer);
    gpio_install_isr_service(0);
    gpio_isr_handler_add(BUTTON_GPIO, vButtonISRHandler, NULL);
}

static void vButtonISRHandler(void* arg) {
    xTimerResetFromISR(xDebounceTimer, NULL);
}

static void vHandleDebounceTimer(TimerHandle_t xTimer) {
    if (gpio_get_level(BUTTON_GPIO) == 1) {
        ESP_LOGI(TAG, "Button confirmed click!");
        if (isMotorRunning) {
            gpio_set_level(MOTOR_CONTROL_GPIO, 0);
        } else {
            gpio_set_level(MOTOR_CONTROL_GPIO, 1);
        }
        isMotorRunning = !isMotorRunning;
    }
}
