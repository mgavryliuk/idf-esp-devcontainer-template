#include "buttons.h"

#include "driver/gpio.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "freertos/timers.h"

static void vConfigure(void);
static void vISRHandler(void* arg);
static void vHandleDebounceTimer(TimerHandle_t xTimer);
static void vHandleWindowTimer(TimerHandle_t xTimer);
static void vHandleButtonEvent(void* pvParameters);

static const char* LOG_PREFIX = "[Buttons]";
static QueueHandle_t gpio_evt_queue = NULL;

static Button_t buttonLeft = {
    .gpio = BUTTON_LEFT_GPIO,
    .eState = BUTTON_STATE_IDLE,
};
static Button_t buttonRight = {
    .gpio = BUTTON_RIGHT_GPIO,
    .eState = BUTTON_STATE_IDLE,
};

void vButtonsCreateTask(void) {
    gpio_evt_queue = xQueueCreate(10, sizeof(ButtonEventMessage_t));
    xTaskCreate(vHandleButtonEvent, "GPIO Event Handler", 4096, NULL, 7, NULL);
    vConfigure();
}

void vButtonsRegisterCallback(uint32_t gpio, ButtonEventCallback_t callback) {
    if (gpio == buttonLeft.gpio) {
        buttonLeft.fnCallback = callback;
    }
    if (gpio == buttonRight.gpio) {
        buttonRight.fnCallback = callback;
    }
}

static void vConfigure(void) {
    gpio_config_t io__conf = {
        .pin_bit_mask = BUTTON_INPUT_PIN_SEL,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_POSEDGE,
    };
    gpio_config(&io__conf);

    buttonLeft.xDebounceTimer = xTimerCreate("Left Button Debounce Timer", pdMS_TO_TICKS(BUTTON_DEBOUNCE_TIME_MS), pdFALSE,
                                             (void*)&buttonLeft, vHandleDebounceTimer);
    buttonRight.xDebounceTimer = xTimerCreate("Right Button Debounce Timer", pdMS_TO_TICKS(BUTTON_DEBOUNCE_TIME_MS), pdFALSE,
                                              (void*)&buttonRight, vHandleDebounceTimer);
    buttonLeft.xWindowTimer =
        xTimerCreate("Left Button Window Timer", pdMS_TO_TICKS(BUTTON_WINDOW_TIME_MS), pdFALSE, (void*)&buttonLeft, vHandleWindowTimer);
    buttonRight.xWindowTimer =
        xTimerCreate("Right Button Window Timer", pdMS_TO_TICKS(BUTTON_WINDOW_TIME_MS), pdFALSE, (void*)&buttonRight, vHandleWindowTimer);

    gpio_install_isr_service(BUTTON_ESP_INTR_FLAG_DEFAULT);
    gpio_isr_handler_add(BUTTON_LEFT_GPIO, vISRHandler, (void*)&buttonLeft);
    gpio_isr_handler_add(BUTTON_RIGHT_GPIO, vISRHandler, (void*)&buttonRight);
}

static void IRAM_ATTR vISRHandler(void* arg) {
    Button_t* pButton = (Button_t*)arg;
    xTimerStopFromISR(pButton->xWindowTimer, NULL);
    xTimerStartFromISR(pButton->xDebounceTimer, NULL);
}

static void vHandleDebounceTimer(TimerHandle_t xTimer) {
    Button_t* pButton = (Button_t*)pvTimerGetTimerID(xTimer);

    if (gpio_get_level(pButton->gpio) == 1) {
        switch (pButton->eState) {
            case BUTTON_STATE_IDLE:
                ESP_LOGD(LOG_PREFIX, "Button on GPIO %d single pressed. Starting register window timer...", pButton->gpio);
                pButton->eState = BUTTON_STATE_SINGLE_PRESSED;
                xTimerStart(pButton->xWindowTimer, 0);
                break;
            case BUTTON_STATE_SINGLE_PRESSED:
                ESP_LOGD(LOG_PREFIX, "Button on GPIO %d double pressed. Triggering event...", pButton->gpio);
                pButton->eState = BUTTON_STATE_IDLE;
                xTimerStop(pButton->xWindowTimer, 0);

                ButtonEventMessage_t msg = {
                    .eEvent = BUTTON_EVENT_DOUBLE_PRESS,
                    .pButton = pButton,
                };
                xQueueSend(gpio_evt_queue, &msg, 0);
                break;
            default:
                break;
        }
    }
}

static void vHandleWindowTimer(TimerHandle_t xTimer) {
    Button_t* pButton = (Button_t*)pvTimerGetTimerID(xTimer);
    if (pButton->eState == BUTTON_STATE_SINGLE_PRESSED) {
        ESP_LOGD(LOG_PREFIX, "Button on GPIO %d single press window expired. Returning to idle state...", pButton->gpio);
        pButton->eState = BUTTON_STATE_IDLE;

        ButtonEventMessage_t msg = {
            .eEvent = BUTTON_EVENT_SINGLE_PRESS,
            .pButton = pButton,
        };
        xQueueSend(gpio_evt_queue, &msg, 0);
    }
}

static void vHandleButtonEvent(void* pvParameters) {
    ButtonEventMessage_t msg;
    while (1) {
        if (xQueueReceive(gpio_evt_queue, &msg, portMAX_DELAY)) {
            if (msg.pButton->fnCallback != NULL) {
                msg.pButton->fnCallback(msg.eEvent, msg.pButton->gpio);
            }
        }
    }
}
