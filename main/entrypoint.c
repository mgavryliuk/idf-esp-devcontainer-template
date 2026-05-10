#include <stdio.h>
#include <string.h>

#include "buzzer_melody.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "led_animator.h"
#include "sdkconfig.h"

const static char* TAG = "[Entrypoint]";

void app_main(void) {
    // adc_oneshot_unit_handle_t adc1_handle;
    // adc_oneshot_unit_init_cfg_t adc1_config = {
    //     .unit_id = ADC_UNIT_1,
    // };
    // ESP_ERROR_CHECK(adc_oneshot_new_unit(&adc1_config, &adc1_handle));

    // adc_oneshot_chan_cfg_t channel_config = {
    //     .atten = ADC_ATTEN_DB_12,
    //     .bitwidth = ADC_BITWIDTH_DEFAULT,
    // };
    // ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, ADC_CHANNEL_3, &channel_config));

    // ESP_LOGI(TAG, "Calibration scheme curve!");
    // esp_err_t ret = ESP_FAIL;
    // adc_cali_handle_t adc1_cali_chan3_handle;
    // adc_cali_curve_fitting_config_t cali_config = {
    //     .unit_id = ADC_UNIT_1,
    //     .atten = ADC_ATTEN_DB_12,
    //     .chan = ADC_CHANNEL_3,
    //     .bitwidth = ADC_BITWIDTH_DEFAULT,
    // };
    // ret = adc_cali_create_scheme_curve_fitting(&cali_config, &adc1_cali_chan3_handle);
    // if (ret == ESP_OK) {
    //     ESP_LOGI(TAG, "Successfully calibrateed!");
    // } else {
    //     ESP_LOGE(TAG, "Calibration failed: %d", ret);
    // }

    // while (1) {
    //     ESP_ERROR_CHECK(adc_oneshot_read(adc1_handle, ADC_CHANNEL_3, &adc_raw[0]));

    //     int transformed_raw = adc_raw[0] * 3300 / ((1 << 12) - 1);
    //     ESP_LOGI(TAG, "ADC%d Channel[%d] Raw Data: %d (%d mV)", ADC_UNIT_1 + 1, ADC_CHANNEL_3, adc_raw[0], transformed_raw);

    //     ESP_ERROR_CHECK(adc_cali_raw_to_voltage(adc1_cali_chan3_handle, adc_raw[0], &voltage[0]));
    //     ESP_LOGI(TAG, "ADC%d Channel[%d] Cali Voltage: %d mV", ADC_UNIT_1 + 1, ADC_CHANNEL_3, voltage[0]);

    //     vTaskDelay(pdMS_TO_TICKS(1000));
    // }

    // ESP_LOGI(TAG, "deregister Curve Fitting calibration scheme");
    // ESP_ERROR_CHECK(adc_cali_delete_scheme_curve_fitting(adc1_cali_chan3_handle));

    // vBuzzerCreateTask();
    // vLedAnimatorCreateTask();
}
