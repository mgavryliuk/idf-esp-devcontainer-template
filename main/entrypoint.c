#include <stdio.h>
#include <string.h>

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

void app_main(void) {
    vPhotoresistorCreateTask();
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
    // vBuzzerCreateTask();
    // vLedAnimatorCreateTask();
}
