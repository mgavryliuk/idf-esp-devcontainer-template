#include <stdio.h>

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "led_blink.h"
#include "rgb_led_blink.h"
#include "sdkconfig.h"

// static const char *LOG_PREFIX = "[ENTRYPOINT]";

void app_main(void) {
    xTaskCreate(LED_PoliceBlinkTask, "LED_PoliceBlinkTask", 4096, NULL, 5, NULL);
    xTaskCreate(RGB_LED_PoliceBlinkTask, "RGB_LED_PoliceBlinkTask", 4096, NULL, 5, NULL);
    while (1) {
        // vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}
