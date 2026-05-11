#include "buzzer_melody.h"

#include "driver/ledc.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static void vConfigure(void);
static void vPlayTone(uint32_t freq_hz, uint32_t duration_ms);
static void vPlayMelody(void);
static void vBuzzerTask(void* pvParameter);

static const char* LOG_PREFIX = "[Buzzer Melody]";
static volatile BuzzerMode_t eCurrentMode = BUZZER_MODE_IDLE;

static const BuzzerNote_t MELODY1[] = {
    {1318, 150}, {1175, 150}, {740, 300}, {830, 300}, {1109, 150}, {987, 150}, {587, 300},
    {659, 300},  {987, 150},  {880, 150}, {554, 300}, {659, 300},  {880, 600},
};

static const BuzzerNote_t MELODY2[] = {
    {660, 150}, {660, 150}, {0, 150}, {660, 150}, {0, 150}, {510, 150}, {660, 150}, {0, 150}, {770, 150}, {0, 300}, {380, 150}, {0, 300},
};
static const BuzzerNote_t MELODY3[] = {
    {330, 600}, {0, 100}, {330, 300}, {392, 300}, {330, 300}, {294, 300}, {262, 700}, {0, 150}, {247, 700}, {0, 200},
    {330, 600}, {0, 100}, {330, 300}, {392, 300}, {330, 300}, {294, 300}, {262, 700}, {0, 150}, {247, 700},
};

static const BuzzerMelody_t MELODIES[] = {
    [BUZZER_MODE_IDLE] =
        {
            .notes = NULL,
            .length = 0,
        },
    [BUZZER_MODE_MELODY1] =
        {
            .notes = MELODY1,
            .length = sizeof(MELODY1) / sizeof(BuzzerNote_t),
        },
    [BUZZER_MODE_MELODY2] =
        {
            .notes = MELODY2,
            .length = sizeof(MELODY2) / sizeof(BuzzerNote_t),
        },
    [BUZZER_MODE_MELODY3] =
        {
            .notes = MELODY3,
            .length = sizeof(MELODY3) / sizeof(BuzzerNote_t),
        },
};

void vBuzzerCreateTask() {
    vConfigure();
    xTaskCreate(vBuzzerTask, "BuzzerTask", 2048, NULL, 5, NULL);
}

void vBuzzerSetMode(BuzzerMode_t mode) {
    if (mode != eCurrentMode) {
        ESP_LOGD(LOG_PREFIX, "Changing buzzer mode from %d to %d", eCurrentMode, mode);
        eCurrentMode = mode;
    }
}

void vBuzzerNextMode(void) {
    eCurrentMode = (eCurrentMode + 1) % BUZZER_MODE_MAX;
    ESP_LOGI(LOG_PREFIX, "Switching to next buzzer mode: %d", eCurrentMode);
}

static void vConfigure(void) {
    ESP_LOGD(LOG_PREFIX, "Configuring buzzer on pin %d", BUZZER_IO);
    ledc_timer_config_t ledc_timer = {
        .speed_mode = BUZZER_MODE,
        .timer_num = BUZZER_TIMER,
        .duty_resolution = BUZZER_DUTY_RESOLUTION,
        .freq_hz = 4000,
        .clk_cfg = BUZZER_CLK,
    };
    ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));
    ledc_channel_config_t ledc_channel_cfg = {
        .gpio_num = BUZZER_IO,
        .channel = BUZZER_CHANNEL,
        .speed_mode = BUZZER_MODE,
        .timer_sel = BUZZER_TIMER,
        .duty = 0,
        .hpoint = 0,
    };
    ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel_cfg));
}

static void vPlayTone(uint32_t freq_hz, uint32_t duration_ms) {
    if (freq_hz > 0) {
        ledc_set_freq(BUZZER_MODE, BUZZER_TIMER, freq_hz);
        ledc_set_duty(BUZZER_MODE, BUZZER_CHANNEL, 258);  // 25% duty cycle for a softer sound
        ledc_update_duty(BUZZER_MODE, BUZZER_CHANNEL);
    } else {
        ledc_set_duty(BUZZER_MODE, BUZZER_CHANNEL, 0);
        ledc_update_duty(BUZZER_MODE, BUZZER_CHANNEL);
    }

    vTaskDelay(pdMS_TO_TICKS(duration_ms));

    // small pause between notes to prevent them from blending together
    ledc_set_duty(BUZZER_MODE, BUZZER_CHANNEL, 0);
    ledc_update_duty(BUZZER_MODE, BUZZER_CHANNEL);
    vTaskDelay(pdMS_TO_TICKS(20));
}

static void vPlayMelody(void) {
    if (eCurrentMode != BUZZER_MODE_IDLE) {
        const BuzzerMelody_t* melody = &MELODIES[eCurrentMode];
        for (uint8_t i = 0; i < melody->length; i++) {
            vPlayTone(melody->notes[i].frequency, melody->notes[i].duration_ms);
        }
    }
    vTaskDelay(pdMS_TO_TICKS(1000));
}

static void vBuzzerTask(void* pvParameter) {
    while (1) {
        vPlayMelody();
    }
}
