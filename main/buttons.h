#ifndef BUTTONS_H
#define BUTTONS_H

#include <stdint.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"

#define BUTTON_LEFT_GPIO 5
#define BUTTON_RIGHT_GPIO 4

#define BUTTON_INPUT_PIN_SEL ((1ULL << BUTTON_LEFT_GPIO) | (1ULL << BUTTON_RIGHT_GPIO))
#define BUTTON_ESP_INTR_FLAG_DEFAULT 0
#define BUTTON_DEBOUNCE_TIME_MS 30
#define BUTTON_WINDOW_TIME_MS 300

typedef enum {
    BUTTON_STATE_IDLE = 0,
    BUTTON_STATE_SINGLE_PRESSED,
    BUTTON_STATE_DOUBLE_PRESSED,
} ButtonState_t;

typedef enum {
    BUTTON_EVENT_SINGLE_PRESS = 0,
    BUTTON_EVENT_DOUBLE_PRESS,
} ButtonEvent_t;

typedef void (*ButtonEventCallback_t)(ButtonEvent_t eEvent, uint8_t gpio);

typedef struct {
    uint8_t gpio;
    ButtonState_t eState;
    TimerHandle_t xDebounceTimer;
    TimerHandle_t xWindowTimer;
    ButtonEventCallback_t fnCallback;
} Button_t;

typedef struct {
    ButtonEvent_t eEvent;
    Button_t* pButton;
} ButtonEventMessage_t;

void vButtonsCreateTask(void);
void vButtonsRegisterCallback(uint32_t gpio, ButtonEventCallback_t callback);

#endif
