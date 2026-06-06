#include "motor.h"

#include "driver/ledc.h"
#include "esp_log.h"

static const char* TAG = "[MOTOR]";

static uint16_t currentDuty = 0;

esp_err_t xConfigureMotor(uint8_t gpio) {
    ESP_LOGI(TAG, "Configuring motor on GPIO %u", gpio);
    ledc_timer_config_t lec_timer = {
        .speed_mode = MOTOR_PWM_MODE,
        .clk_cfg = MOTOR_PWM_CLOCK,
        .duty_resolution = MOTOR_PWM_DUTY_RESOLUTION,
        .freq_hz = MOTOR_PWM_FREQ_HZ,
        .timer_num = MOTOR_PWM_TIMER,
    };
    ESP_ERROR_CHECK(ledc_timer_config(&lec_timer));

    ledc_channel_config_t ledc_channel_cfg = {
        .gpio_num = gpio,
        .channel = MOTOR_PWM_CHANNEL,
        .speed_mode = MOTOR_PWM_MODE,
        .timer_sel = MOTOR_PWM_TIMER,
        .duty = currentDuty,
        .hpoint = 0,
    };
    ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel_cfg));

    return ESP_OK;
}

esp_err_t xIncreaseSpeed(uint16_t amount) {
    if (currentDuty >= MOTOR_MAX_SPEED) {
        return ESP_OK;
    }

    ESP_LOGI(TAG, "Increasing speed by %d", amount);
    currentDuty += amount;
    if (currentDuty > MOTOR_MAX_SPEED) {
        currentDuty = MOTOR_MAX_SPEED;
    }

    if (currentDuty < MOTOR_MIN_SPEED) {
        currentDuty = MOTOR_MIN_SPEED;
    }
    ESP_LOGI(TAG, "New speed is %d", currentDuty);

    ledc_set_duty(MOTOR_PWM_MODE, MOTOR_PWM_CHANNEL, currentDuty);
    ledc_update_duty(MOTOR_PWM_MODE, MOTOR_PWM_CHANNEL);
    return ESP_OK;
}

esp_err_t xDecreaseSpeed(uint16_t amount) {
    if (currentDuty == MOTOR_STOP_SPEED) {
        return ESP_OK;
    }
    ESP_LOGI(TAG, "Decreasing speed by %d", amount);

    if ((amount > currentDuty) || ((currentDuty - amount) < MOTOR_MIN_SPEED)) {
        currentDuty = MOTOR_STOP_SPEED;
    } else {
        currentDuty -= amount;
    }
    ESP_LOGI(TAG, "New speed is %d", currentDuty);

    ledc_set_duty(MOTOR_PWM_MODE, MOTOR_PWM_CHANNEL, currentDuty);
    ledc_update_duty(MOTOR_PWM_MODE, MOTOR_PWM_CHANNEL);
    return ESP_OK;
}
