#include <stdio.h>
#include <string.h>

#include "buttons.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#include "motor.h"
#include "sdkconfig.h"

#define MOTOR_CONTROL_GPIO (11ULL)
#define MOTOR_CONTROL_GPIO_MASK (1ULL << MOTOR_CONTROL_GPIO)

#define RIGHT_BUTTON_GPIO (4ULL)
#define LEFT_BUTTON_GPIO (5ULL)

#define MOTOR_SPEED_STEP (50U)

static void vHandleLeftButton(Button_t* button);
static void vHandleRightButton(Button_t* button);

void app_main(void) {
    gpio_install_isr_service(0);
    xConfigureMotor(MOTOR_CONTROL_GPIO);

    ButtonCfg_t left_button = {
        .gpio = LEFT_BUTTON_GPIO,
        .fnCallback = &vHandleLeftButton,
    };
    xButtonConfigure(&left_button);

    ButtonCfg_t right_button = {
        .gpio = RIGHT_BUTTON_GPIO,
        .fnCallback = &vHandleRightButton,
    };
    xButtonConfigure(&right_button);

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

static void vHandleLeftButton(Button_t* button) {
    xDecreaseSpeed(MOTOR_SPEED_STEP);
}

static void vHandleRightButton(Button_t* button) {
    xIncreaseSpeed(MOTOR_SPEED_STEP);
}
