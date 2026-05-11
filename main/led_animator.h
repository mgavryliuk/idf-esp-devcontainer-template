#ifndef LED_ANIMATOR_H
#define LED_ANIMATOR_H

#include <stdint.h>

#define LED_BLUE_GPIO 7
#define LED_RED_GPIO 6

typedef enum {
    LED_MODE_IDLE = 0,
    LED_MODE_SEQUENTIAL,
    LED_MODE_SOS,
    LED_MODE_POLICE,
    LED_MODE_MAX,
} LedMode_t;

typedef struct {
    uint8_t blue_state;
    uint8_t red_state;
    uint32_t duration_ms;
} LedStep_t;

typedef struct {
    const LedStep_t* steps;
    uint8_t amount;
} LedPattern_t;

void vLedAnimatorCreateTask();
void vLedAnimatorSetMode(LedMode_t mode);
void vLedAnimatorNextMode(void);

#endif
