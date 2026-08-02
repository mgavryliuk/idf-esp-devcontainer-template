#include <stdio.h>
#include <string.h>

#include "driver/pulse_cnt.h"
#include "esp_err.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "sdkconfig.h"
#include "soc/gpio_reg.h"

#define ENCODER_GPIO_A 5
#define ENCODER_GPIO_B 4
#define PCNT_UNIT PCNT_UNIT_0

const static char* TAG = "[Entrypoint]";
pcnt_unit_handle_t pcnt_unit = NULL;
pcnt_channel_handle_t pcnt_channel_0 = NULL;
pcnt_channel_handle_t pcnt_channel_1 = NULL;

static int32_t last_counter = 0;
static int32_t remainder = 0;

void app_main(void) {
    pcnt_unit_config_t unit_config = {
        .high_limit = 10000,
        .low_limit = -10000,
        .flags.accum_count = true,
    };
    ESP_ERROR_CHECK(pcnt_new_unit(&unit_config, &pcnt_unit));

    pcnt_chan_config_t chan_config_0 = {
        .edge_gpio_num = ENCODER_GPIO_A,
        .level_gpio_num = ENCODER_GPIO_B,
    };
    ESP_ERROR_CHECK(pcnt_new_channel(pcnt_unit, &chan_config_0, &pcnt_channel_0));
    ESP_ERROR_CHECK(pcnt_channel_set_edge_action(pcnt_channel_0, PCNT_CHANNEL_EDGE_ACTION_INCREASE, PCNT_CHANNEL_EDGE_ACTION_DECREASE));
    ESP_ERROR_CHECK(pcnt_channel_set_level_action(pcnt_channel_0, PCNT_CHANNEL_LEVEL_ACTION_KEEP, PCNT_CHANNEL_LEVEL_ACTION_INVERSE));

    pcnt_chan_config_t chan_config_1 = {
        .edge_gpio_num = ENCODER_GPIO_B,
        .level_gpio_num = ENCODER_GPIO_A,
    };
    ESP_ERROR_CHECK(pcnt_new_channel(pcnt_unit, &chan_config_1, &pcnt_channel_1));
    ESP_ERROR_CHECK(pcnt_channel_set_edge_action(pcnt_channel_1, PCNT_CHANNEL_EDGE_ACTION_DECREASE, PCNT_CHANNEL_EDGE_ACTION_INCREASE));
    ESP_ERROR_CHECK(pcnt_channel_set_level_action(pcnt_channel_1, PCNT_CHANNEL_LEVEL_ACTION_KEEP, PCNT_CHANNEL_LEVEL_ACTION_INVERSE));

    pcnt_glitch_filter_config_t filter_config = {
        .max_glitch_ns = 1000,
    };
    ESP_ERROR_CHECK(pcnt_unit_set_glitch_filter(pcnt_unit, &filter_config));
    ESP_ERROR_CHECK(pcnt_unit_enable(pcnt_unit));
    ESP_ERROR_CHECK(pcnt_unit_clear_count(pcnt_unit));
    ESP_ERROR_CHECK(pcnt_unit_start(pcnt_unit));
    ESP_LOGI(TAG, "PCNT X4 Quadrature Mode Initialized");
    while (1) {
        int current_counter = 0;
        ESP_ERROR_CHECK(pcnt_unit_get_count(pcnt_unit, &current_counter));
        int diff = current_counter - last_counter;
        last_counter = current_counter;

        if (diff != 0) {
            remainder += diff;
            int8_t clicks = remainder / 4;
            remainder = remainder % 4;
            ESP_LOGI(TAG, "Encoder was increase (positive number) or decreased (negative) by %d points", clicks);
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}
