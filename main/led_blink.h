#ifndef LED_BLINK_H
#define LED_BLINK_H

#include <stdint.h>

#define LED_RED_GPIO 4
#define LED_BLUE_GPIO 5
#define POLICE_BLINK_DELAY 280

typedef struct {
    uint8_t led_gpio;
    uint8_t state;
} LedWithState;

void LED_PoliceBlinkTask(void* pvParameter);

#endif
