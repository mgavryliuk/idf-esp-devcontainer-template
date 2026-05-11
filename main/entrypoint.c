#include <stdio.h>
#include <string.h>

#include "buttons.h"
#include "buzzer_melody.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "led_animator.h"
#include "photoresistor.h"
#include "sdkconfig.h"

const static char* TAG = "[Entrypoint]";

void vButtonCallback(ButtonEvent_t eEvent, uint8_t gpio) {
    ESP_LOGI(TAG, "Button callback triggered for GPIO %d with event %d", gpio, eEvent);
    switch (eEvent) {
        case BUTTON_EVENT_SINGLE_PRESS:
            if (gpio == BUTTON_LEFT_GPIO)
                vLedAnimatorNextMode();

            if (gpio == BUTTON_RIGHT_GPIO)
                vBuzzerNextMode();
            break;
        case BUTTON_EVENT_DOUBLE_PRESS:
            if (gpio == BUTTON_LEFT_GPIO)
                vLedAnimatorSetMode(LED_MODE_IDLE);

            if (gpio == BUTTON_RIGHT_GPIO)
                vBuzzerSetMode(BUZZER_MODE_IDLE);
            break;
        default:
            break;
    }
}

void app_main(void) {
    vBuzzerCreateTask();
    vLedAnimatorCreateTask();
    vPhotoresistorCreateTask();

    vButtonsCreateTask();
    vButtonsRegisterCallback(BUTTON_LEFT_GPIO, vButtonCallback);
    vButtonsRegisterCallback(BUTTON_RIGHT_GPIO, vButtonCallback);
}
