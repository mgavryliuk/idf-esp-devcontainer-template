#include <stdio.h>
#include <string.h>

#include "driver/gpio.h"
#include "driver/touch_sens.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "sdkconfig.h"
#include "soc/touch_sensor_channel.h"

#define TOUCH_PAD_NUM TOUCH_PAD_NUM11_GPIO_NUM
#define BASELINE_SAMPLES_NUM (32)
#define THERSHOLD_RATIO (0.15f)

#define LED_GPIO 7

const static char* TAG = "[Entrypoint]";

esp_err_t eLedInit(void) {
    gpio_reset_pin(LED_GPIO);
    gpio_set_direction(LED_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_level(LED_GPIO, 0);
    return ESP_OK;
}

bool example_touch_on_active_callback(touch_sensor_handle_t sens_handle, const touch_active_event_data_t* event, void* user_ctx) {
    ESP_EARLY_LOGI(TAG, "[CH %d] active", (int)event->chan_id);
    gpio_set_level(LED_GPIO, 1);
    return false;
}

bool example_touch_on_inactive_callback(touch_sensor_handle_t sens_handle, const touch_inactive_event_data_t* event, void* user_ctx) {
    ESP_EARLY_LOGI(TAG, "[CH %d] inactive", (int)event->chan_id);
    gpio_set_level(LED_GPIO, 0);
    return false;
}

void app_main(void) {
    eLedInit();

    touch_chan_info_t chan_info = {};
    touch_sensor_handle_t sens_handle = NULL;
    touch_channel_handle_t chan_handle = NULL;

    touch_sensor_sample_config_t sample_cfg = TOUCH_SENSOR_V2_DEFAULT_SAMPLE_CONFIG(500, TOUCH_VOLT_LIM_L_0V5, TOUCH_VOLT_LIM_H_2V2);
    touch_sensor_config_t touch_cfg = TOUCH_SENSOR_DEFAULT_BASIC_CONFIG(CONFIG_SOC_TOUCH_SAMPLE_CFG_NUM, &sample_cfg);
    ESP_ERROR_CHECK(touch_sensor_new_controller(&touch_cfg, &sens_handle));

    touch_channel_config_t chan_cfg = {
        .active_thresh = {2000},
        .charge_speed = TOUCH_CHARGE_SPEED_7,
        .init_charge_volt = TOUCH_INIT_CHARGE_VOLT_DEFAULT,
    };
    // Allocate a new touch sensor controller handle
    ESP_ERROR_CHECK(touch_sensor_new_channel(sens_handle, TOUCH_PAD_NUM, &chan_cfg, &chan_handle));
    ESP_ERROR_CHECK(touch_sensor_get_channel_info(chan_handle, &chan_info));
    ESP_LOGI(TAG, "Touch [CH %d] enabled on GPIO%d", TOUCH_PAD_NUM, chan_info.chan_gpio);

    uint32_t benchmark;
    uint32_t benchmark_amount = 0;
    ESP_ERROR_CHECK(touch_sensor_enable(sens_handle));
    for (int i = 0; i < BASELINE_SAMPLES_NUM; i++) {
        ESP_ERROR_CHECK(touch_sensor_trigger_oneshot_scanning(sens_handle, 2000));
        ESP_ERROR_CHECK(touch_channel_read_data(chan_handle, TOUCH_CHAN_DATA_TYPE_BENCHMARK, &benchmark));
        benchmark_amount += benchmark;
        vTaskDelay(pdMS_TO_TICKS(20));
    }
    ESP_ERROR_CHECK(touch_sensor_disable(sens_handle));
    uint32_t avg_benchmark = (benchmark_amount / BASELINE_SAMPLES_NUM);
    chan_cfg.active_thresh[0] = avg_benchmark * THERSHOLD_RATIO;
    ESP_LOGI(TAG, "AVG Benchmark: %" PRIu32 "; Threshold: %" PRIu32 "", avg_benchmark, chan_cfg.active_thresh[0]);
    /* Update the channel configuration */
    ESP_ERROR_CHECK(touch_sensor_reconfig_channel(chan_handle, &chan_cfg));

    touch_event_callbacks_t callbacks = {
        .on_active = example_touch_on_active_callback,
        .on_inactive = example_touch_on_inactive_callback,
    };
    ESP_ERROR_CHECK(touch_sensor_register_callbacks(sens_handle, &callbacks, NULL));

    ESP_ERROR_CHECK(touch_sensor_enable(sens_handle));
    ESP_ERROR_CHECK(touch_sensor_start_continuous_scanning(sens_handle));

    uint32_t result;
    while (1) {
        ESP_LOGD(TAG, "Cycle...");
        ESP_ERROR_CHECK(touch_channel_read_data(chan_handle, TOUCH_CHAN_DATA_TYPE_BENCHMARK, &result));
        vTaskDelay(pdMS_TO_TICKS(20));
    }
}
