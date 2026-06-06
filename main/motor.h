#ifndef MOTOR_H
#define MOTOR_H

#include "driver/ledc.h"

#define MOTOR_STOP_SPEED (0U)
#define MOTOR_MIN_SPEED (512U)
#define MOTOR_MAX_SPEED ((1 << 10) - 1U)

#define MOTOR_PWM_FREQ_HZ (10000)
#define MOTOR_PWM_CHANNEL LEDC_CHANNEL_0
#define MOTOR_PWM_TIMER LEDC_TIMER_0
#define MOTOR_PWM_MODE LEDC_LOW_SPEED_MODE
#define MOTOR_PWM_DUTY_RESOLUTION LEDC_TIMER_10_BIT
#define MOTOR_PWM_CLOCK LEDC_AUTO_CLK

esp_err_t xConfigureMotor(uint8_t gpio);
esp_err_t xIncreaseSpeed(uint16_t amount);
esp_err_t xDecreaseSpeed(uint16_t amount);

#endif
