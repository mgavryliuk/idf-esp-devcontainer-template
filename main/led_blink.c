#include "led_blink.h"

#include "driver/gpio.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"

static void LED_Configure(void);
static void LED_Toggle(LedWithState* led);

static const char* LOG_PREFIX = "[LED Blink]";

static LedWithState RedLed = {
    .led_gpio = LED_RED_GPIO,
    .state = 0,
};

static LedWithState BlueLed = {
    .led_gpio = LED_BLUE_GPIO,
    .state = 0,
};

void LED_PoliceBlinkTask(void* pvParameter) {
    LED_Configure();

    while (1) {
        ESP_LOGD(LOG_PREFIX, "Turning Red LED ON!");
        LED_Toggle(&RedLed);
        vTaskDelay(POLICE_BLINK_DELAY / portTICK_PERIOD_MS);
        LED_Toggle(&RedLed);
        ESP_LOGD(LOG_PREFIX, "Turning Blue LED OFF!");
        ESP_LOGD(LOG_PREFIX, "Turning Blue LED ON!");
        LED_Toggle(&BlueLed);
        vTaskDelay(POLICE_BLINK_DELAY / portTICK_PERIOD_MS);
        ESP_LOGD(LOG_PREFIX, "Turning Blue LED OFF!");
        LED_Toggle(&BlueLed);
    }
}

static void LED_Configure(void) {
    ESP_LOGD(LOG_PREFIX, "Example configured to blink GPIO LED!");
    gpio_reset_pin(RedLed.led_gpio);
    gpio_set_direction(RedLed.led_gpio, GPIO_MODE_OUTPUT);
    gpio_reset_pin(BlueLed.led_gpio);
    gpio_set_direction(BlueLed.led_gpio, GPIO_MODE_OUTPUT);
}

static void LED_Toggle(LedWithState* led) {
    gpio_set_level(led->led_gpio, led->state);
    led->state = !led->state;
}
