#include "led_animator.h"

#include "driver/gpio.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static void vLedAnimatorTask(void* pvParameter);
static void vRunBlinkPattern(uint8_t cycle_amount);
static void vConfigure(void);

static const char* LOG_PREFIX = "[LED Animator]";

static const LedStep_t SEQUENTIAL_STEPS[] = {
    {.blue_state = 1, .red_state = 0, .duration_ms = 500},
    {.blue_state = 0, .red_state = 1, .duration_ms = 500},
};

static const LedStep_t SOS_STEPS[] = {
    {.blue_state = 1, .red_state = 1, .duration_ms = 200}, {.blue_state = 0, .red_state = 0, .duration_ms = 200},
    {.blue_state = 1, .red_state = 1, .duration_ms = 200}, {.blue_state = 0, .red_state = 0, .duration_ms = 200},
    {.blue_state = 1, .red_state = 1, .duration_ms = 200}, {.blue_state = 0, .red_state = 0, .duration_ms = 600},

    {.blue_state = 1, .red_state = 1, .duration_ms = 600}, {.blue_state = 0, .red_state = 0, .duration_ms = 600},
    {.blue_state = 1, .red_state = 1, .duration_ms = 600}, {.blue_state = 0, .red_state = 0, .duration_ms = 600},
    {.blue_state = 1, .red_state = 1, .duration_ms = 600}, {.blue_state = 0, .red_state = 0, .duration_ms = 600},

    {.blue_state = 1, .red_state = 1, .duration_ms = 200}, {.blue_state = 0, .red_state = 0, .duration_ms = 200},
    {.blue_state = 1, .red_state = 1, .duration_ms = 200}, {.blue_state = 0, .red_state = 0, .duration_ms = 200},
    {.blue_state = 1, .red_state = 1, .duration_ms = 200}, {.blue_state = 0, .red_state = 0, .duration_ms = 1400},
};

static const LedStep_t POLICE_STEPS[] = {
    {.blue_state = 1, .red_state = 0, .duration_ms = 100}, {.blue_state = 0, .red_state = 0, .duration_ms = 100},
    {.blue_state = 1, .red_state = 0, .duration_ms = 100}, {.blue_state = 0, .red_state = 0, .duration_ms = 200},
    {.blue_state = 0, .red_state = 1, .duration_ms = 100}, {.blue_state = 0, .red_state = 0, .duration_ms = 100},
    {.blue_state = 0, .red_state = 1, .duration_ms = 100}, {.blue_state = 0, .red_state = 0, .duration_ms = 200},
};

static const LedPattern_t PATTERNS[] = {
    [LED_MODE_IDLE] = {NULL, 0},
    [LED_MODE_SEQUENTIAL] = {SEQUENTIAL_STEPS, sizeof(SEQUENTIAL_STEPS) / sizeof(LedStep_t)},
    [LED_MODE_SOS] = {SOS_STEPS, sizeof(SOS_STEPS) / sizeof(LedStep_t)},
    [LED_MODE_POLICE] = {POLICE_STEPS, sizeof(POLICE_STEPS) / sizeof(LedStep_t)},
};

static volatile LedMode_t eCurrentMode = LED_MODE_IDLE;

void vLedAnimatorCreateTask() {
    vConfigure();
    xTaskCreate(vLedAnimatorTask, "LedAnimatorTask", 4096, NULL, 5, NULL);
}

void vLedAnimatorSetMode(LedMode_t mode) {
    if (mode != eCurrentMode) {
        ESP_LOGD(LOG_PREFIX, "Changing LED mode from %d to %d", eCurrentMode, mode);
        eCurrentMode = mode;
    }
}

static void vConfigure(void) {
    ESP_LOGD(LOG_PREFIX, "Configuring Leds on pins %d (red) and %d (blue)", LED_RED_GPIO, LED_BLUE_GPIO);
    gpio_reset_pin(LED_RED_GPIO);
    gpio_set_direction(LED_RED_GPIO, GPIO_MODE_OUTPUT);
    gpio_reset_pin(LED_BLUE_GPIO);
    gpio_set_direction(LED_BLUE_GPIO, GPIO_MODE_OUTPUT);
}

static void vRunBlinkPattern(uint8_t cycle_amount) {
    if (eCurrentMode == LED_MODE_IDLE) {
        gpio_set_level(LED_RED_GPIO, 0);
        gpio_set_level(LED_BLUE_GPIO, 0);
        return;
    }

    const LedPattern_t* pattern = &PATTERNS[eCurrentMode];
    for (uint8_t cycle = 0; cycle < cycle_amount; cycle++) {
        for (size_t i = 0; i < pattern->amount; i++) {
            gpio_set_level(LED_RED_GPIO, pattern->steps[i].red_state);
            gpio_set_level(LED_BLUE_GPIO, pattern->steps[i].blue_state);
            vTaskDelay(pdMS_TO_TICKS(pattern->steps[i].duration_ms));
        }
    }
}

static void vLedAnimatorTask(void* pvParameter) {
    LedMode_t mode;
    while (1) {
        mode = (eCurrentMode) % (LED_MODE_MAX - 1) + 1;
        vLedAnimatorSetMode(mode);
        vRunBlinkPattern(4);
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}
