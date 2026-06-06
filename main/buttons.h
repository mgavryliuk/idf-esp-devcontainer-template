#ifndef BUTTON_H
#define BUTTON_H

#include <stdint.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"

#define BUTTONS_MAX_AMOUNT (4)
#define BUTTON_DEBOUNCE_TIME_MS (30)

struct Button_t;

typedef void (*ButtonEventCallback_t)(struct Button_t* button);

typedef struct {
    uint8_t gpio;
    ButtonEventCallback_t fnCallback;
} ButtonCfg_t;

typedef struct Button_t {
    ButtonCfg_t cfg;
    TimerHandle_t xDebounceTimer;
} Button_t;

esp_err_t xButtonConfigure(ButtonCfg_t* cfg);

#endif
