#include "photoresistor.h"

#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static void vDoMeasurements(void);
static void vPhotoresistorTask(void* pvParameters);

static const char* LOG_PREFIX = "[Photoresistor]";

void vPhotoresistorCreateTask() {
    xTaskCreate(vPhotoresistorTask, "Photoresistor Task", 4096, NULL, 5, NULL);
}

static void vDoMeasurements() {
    ESP_LOGD(LOG_PREFIX, "Configuring ADC...");
    adc_oneshot_unit_handle_t adc1_handle;
    adc_oneshot_unit_init_cfg_t adc1_config = {
        .unit_id = PHOTORESISTOR_ADC_UNIT,
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&adc1_config, &adc1_handle));

    adc_oneshot_chan_cfg_t channel_config = {
        .atten = PHOTORESISTOR_ADC_ATTEN,
        .bitwidth = PHOTORESISTOR_ADC_BITWIDTH,
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, PHOTORESISTOR_ADC_CHANNEL, &channel_config));

    ESP_LOGD(LOG_PREFIX, "Registering Curve Fitting calibration scheme");
    adc_cali_handle_t adc1_cali_chan3_handle;
    adc_cali_curve_fitting_config_t cali_config = {
        .unit_id = PHOTORESISTOR_ADC_UNIT,
        .atten = PHOTORESISTOR_ADC_ATTEN,
        .chan = PHOTORESISTOR_ADC_CHANNEL,
        .bitwidth = PHOTORESISTOR_ADC_BITWIDTH,
    };
    ESP_ERROR_CHECK(adc_cali_create_scheme_curve_fitting(&cali_config, &adc1_cali_chan3_handle));

    ESP_LOGD(LOG_PREFIX, "Starting ADC measurements");
    int result_raw;
    int result_raw_sum = 0;
    int result_raw_avg = 0;
    int result;
    int resistance = 0;

    for (uint8_t i = 0; i < PHOTORESISTOR_SAMPLING_AMOUNT; i++) {
        ESP_ERROR_CHECK(adc_oneshot_read(adc1_handle, PHOTORESISTOR_ADC_CHANNEL, &result_raw));
        result_raw_sum += result_raw;
    }
    result_raw_avg = result_raw_sum / PHOTORESISTOR_SAMPLING_AMOUNT;
    ESP_LOGD(LOG_PREFIX, "Raw ADC%d Channel[%d] Average: %d", PHOTORESISTOR_ADC_UNIT + 1, PHOTORESISTOR_ADC_CHANNEL, result_raw_avg);

    ESP_ERROR_CHECK(adc_cali_raw_to_voltage(adc1_cali_chan3_handle, result_raw_avg, &result));
    if (result < ADC_VREF) {
        resistance = result * PHOTORESISTOR_FIXED_RESISTANCE_OHMS / (ADC_VREF - result);
    } else {
        resistance = -1;  // Або дуже велике число (повна темрява/світло)
    }
    ESP_LOGI(LOG_PREFIX, "ADC%d Channel[%d] Cali Voltage: %d mV (%d Ohms)", PHOTORESISTOR_ADC_UNIT + 1, PHOTORESISTOR_ADC_CHANNEL, result,
             resistance);

    ESP_LOGD(LOG_PREFIX, "Cleaning up ADC");
    ESP_ERROR_CHECK(adc_oneshot_del_unit(adc1_handle));
    ESP_LOGD(LOG_PREFIX, "Deregister Curve Fitting calibration scheme");
    ESP_ERROR_CHECK(adc_cali_delete_scheme_curve_fitting(adc1_cali_chan3_handle));
}

static void vPhotoresistorTask(void* pvParameters) {
    while (1) {
        vDoMeasurements();
        vTaskDelay(pdMS_TO_TICKS(PHOTORESISTOR_SLEEP_TIME_MS));
    }
}
