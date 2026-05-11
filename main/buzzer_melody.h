#ifndef BUZZER_H
#define BUZZER_H

#include <stdint.h>

#define BUZZER_MODE LEDC_LOW_SPEED_MODE
#define BUZZER_TIMER LEDC_TIMER_0
#define BUZZER_CHANNEL LEDC_CHANNEL_0
#define BUZZER_DUTY_RESOLUTION LEDC_TIMER_10_BIT
#define BUZZER_CLK LEDC_AUTO_CLK
#define BUZZER_IO 2

typedef enum {
    BUZZER_MODE_IDLE = 0,
    BUZZER_MODE_MELODY1,
    BUZZER_MODE_MELODY2,
    BUZZER_MODE_MELODY3,
    BUZZER_MODE_MAX,
} BuzzerMode_t;

typedef struct {
    uint32_t frequency;
    uint32_t duration_ms;
} BuzzerNote_t;

typedef struct {
    const BuzzerNote_t* notes;
    uint8_t length;
} BuzzerMelody_t;

void vBuzzerCreateTask();
void vBuzzerSetMode(BuzzerMode_t mode);
void vBuzzerNextMode(void);

#endif
