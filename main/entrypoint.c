#include <stdio.h>
#include <string.h>

#include "driver/ledc.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_err.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "sdkconfig.h"

#define MAX_ADC_MV 3300

#define POT_ADC_CHANNEL ADC_CHANNEL_4
#define POT_ADC_UNIT ADC_UNIT_1
#define POT_ADC_BITWIDTH ADC_BITWIDTH_12
#define POT_ADC_ATTEN ADC_ATTEN_DB_12

#define SERVO_LEDC_SPEED_MODE LEDC_LOW_SPEED_MODE
#define SERVO_LEDC_TIMER LEDC_TIMER_0
#define SERVO_LEDC_RESOLUTION LEDC_TIMER_13_BIT
#define SERVO_LEDC_CHANNEL LEDC_CHANNEL_0
#define SERVO_LEDC_FREQUENCY 50
#define SERVO_LEDC_GPIO 4

// TODO: find proper angle to duty map
#define SERVO_LEDC_MIN_ANGLE_DUTY 205
#define SERVO_LEDC_MAX_ANGLE_DUTY 1025
#define SERVO_LEDC_MIDDLE_ANGLE_DUTY 615

static esp_err_t pot_configure(void);
static esp_err_t servo_configure(void);

const static char* TAG = "[Entrypoint]";
static adc_oneshot_unit_handle_t pot_adc_handle;
static adc_cali_handle_t pot_adc_cali_handle;

void app_main(void) {
    const uint8_t samples = 8;

    pot_configure();
    servo_configure();
    while (1) {
        int sum = 0;

        for (uint8_t i = 0; i < samples; i++) {
            int raw_value = 0;
            int cali_value = 0;
            adc_oneshot_read(pot_adc_handle, POT_ADC_CHANNEL, &raw_value);
            ESP_ERROR_CHECK(adc_cali_raw_to_voltage(pot_adc_cali_handle, raw_value, &cali_value));
            sum += cali_value;
        };
        int average = sum / samples;

        if (average > MAX_ADC_MV)
            average = MAX_ADC_MV;

        ESP_LOGI(TAG, "Average calibrated value: %d mV", average);
        int duty = (average * (SERVO_LEDC_MAX_ANGLE_DUTY - SERVO_LEDC_MIN_ANGLE_DUTY)) / MAX_ADC_MV + SERVO_LEDC_MIN_ANGLE_DUTY;
        ESP_LOGI(TAG, "Duty: %d mV", duty);

        if (duty < SERVO_LEDC_MIN_ANGLE_DUTY)
            duty = SERVO_LEDC_MIN_ANGLE_DUTY;
        if (duty > SERVO_LEDC_MAX_ANGLE_DUTY)
            duty = SERVO_LEDC_MAX_ANGLE_DUTY;

        ledc_set_duty(SERVO_LEDC_SPEED_MODE, SERVO_LEDC_CHANNEL, duty);
        ledc_update_duty(SERVO_LEDC_SPEED_MODE, SERVO_LEDC_CHANNEL);

        int angle = ((duty - SERVO_LEDC_MIDDLE_ANGLE_DUTY) * 90) / (SERVO_LEDC_MAX_ANGLE_DUTY - SERVO_LEDC_MIDDLE_ANGLE_DUTY);
        ESP_LOGI(TAG, "Angle from center: %d degrees", angle);

        ESP_LOGI(TAG, "Cycle complete...");
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

static esp_err_t pot_configure(void) {
    adc_oneshot_unit_init_cfg_t init_config = {
        .unit_id = POT_ADC_UNIT,
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config, &pot_adc_handle));

    adc_oneshot_chan_cfg_t pot_chan_config = {
        .bitwidth = POT_ADC_BITWIDTH,
        .atten = POT_ADC_ATTEN,
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(pot_adc_handle, POT_ADC_CHANNEL, &pot_chan_config));

    adc_cali_curve_fitting_config_t cali_config = {
        .unit_id = POT_ADC_UNIT,
        .bitwidth = POT_ADC_BITWIDTH,
        .atten = POT_ADC_ATTEN,
    };
    ESP_ERROR_CHECK(adc_cali_create_scheme_curve_fitting(&cali_config, &pot_adc_cali_handle));
    return ESP_OK;
}

static esp_err_t servo_configure(void) {
    ledc_timer_config_t servo_timer_cfg = {
        .speed_mode = SERVO_LEDC_SPEED_MODE,
        .duty_resolution = SERVO_LEDC_RESOLUTION,
        .timer_num = SERVO_LEDC_TIMER,
        .freq_hz = SERVO_LEDC_FREQUENCY,
        .clk_cfg = LEDC_AUTO_CLK,
    };
    ESP_ERROR_CHECK(ledc_timer_config(&servo_timer_cfg));

    ledc_channel_config_t servo_chan_cfg = {
        .speed_mode = SERVO_LEDC_SPEED_MODE,
        .channel = SERVO_LEDC_CHANNEL,
        .timer_sel = SERVO_LEDC_TIMER,
        .gpio_num = SERVO_LEDC_GPIO,
        .duty = SERVO_LEDC_MIDDLE_ANGLE_DUTY,
        .hpoint = 0,
    };
    ESP_ERROR_CHECK(ledc_channel_config(&servo_chan_cfg));

    return ESP_OK;
}
