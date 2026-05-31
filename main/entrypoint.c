#include <stdio.h>
#include <string.h>

#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "sdkconfig.h"

const static char* TAG = "[Entrypoint]";

void app_main(void) {
    while (1) {
        ESP_LOGD(TAG, "Cycle...");
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}
