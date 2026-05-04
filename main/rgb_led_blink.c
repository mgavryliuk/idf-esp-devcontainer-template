#include "rgb_led_blink.h"

#include "driver/gpio.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "led_strip.h"

static void RGB_LED_Configure(void);
static void RGB_LED_Toggle(void);

static const char* LOG_PREFIX = "[RGB LED Blink]";

static led_strip_handle_t ledStrip;
static uint8_t isRed = 0;

void RGB_LED_PoliceBlinkTask(void* pvParameter) {
    RGB_LED_Configure();
    while (1) {
        RGB_LED_Toggle();
        vTaskDelay(POLICE_BLINK_DELAY / portTICK_PERIOD_MS);
    }
};

static void RGB_LED_Configure(void) {
    ESP_LOGD(LOG_PREFIX, "Configuring RGB LED on GPIO %d", RGB_LED_GPIO);
    led_strip_config_t strip_config = {
        .strip_gpio_num = RGB_LED_GPIO,
        .max_leds = 1,
    };
    led_strip_spi_config_t spi_config = {
        .spi_bus = SPI2_HOST,
        .flags.with_dma = true,
    };
    ESP_ERROR_CHECK(led_strip_new_spi_device(&strip_config, &spi_config, &ledStrip));
    led_strip_clear(ledStrip);
}

static void RGB_LED_Toggle(void) {
    static uint32_t red_color, green_color, blue_color;

    if (isRed) {
        red_color = 0;
        green_color = 0;
        blue_color = 50;
    } else {
        red_color = 50;
        green_color = 0;
        blue_color = 0;
    }
    isRed = !isRed;

    led_strip_set_pixel(ledStrip, 0, red_color, green_color, blue_color);
    led_strip_refresh(ledStrip);
}
