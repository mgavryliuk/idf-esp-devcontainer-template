#include <stdio.h>
#include <string.h>

#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_private/periph_ctrl.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "sdkconfig.h"
#include "soc/gpio_reg.h"
#include "soc/io_mux_reg.h"
#include "soc/rmt_reg.h"
#include "soc/system_reg.h"

#define RTM_GPIO (4)             // use func 0/1
#define RTM_SIG_OUT0_NUM (0x51)  // 81
#define DEFAULT_REG_SIZE (0x4)
#define GPIO_MATRIX_OUT_SEL_FIELD_MASK (0x1FF)
#define GPIO_MATRIX_OEN_SEL_FIELD_MASK (BIT(10))

#define RTM_SCLK_FREQ_MAX (40000000)
#define RTM_CHAN_CLK_FREQ_MIN (RTM_SCLK_FREQ_MAX / 255)

const static char* TAG = "[MAIN]";

static uint32_t reg_val;

esp_err_t rmt_tx_raw_init(int gpio, int rmt_channel_tx, uint32_t resolution_hz) {
    if ((gpio > 21 && gpio < 26) || gpio > 48 || gpio < 0) {
        return ESP_FAIL;
    }

    if (rmt_channel_tx < 0 || rmt_channel_tx > 3) {
        return ESP_FAIL;
    }

    if (resolution_hz > RTM_SCLK_FREQ_MAX || resolution_hz < RTM_CHAN_CLK_FREQ_MIN) {
        return ESP_FAIL;
    }

    // Enable RMT peripheral
    if (!REG_GET_BIT(SYSTEM_PERIP_CLK_EN0_REG, SYSTEM_RMT_CLK_EN)) {
        REG_SET_BIT(SYSTEM_PERIP_RST_EN0_REG, SYSTEM_RMT_RST);
        REG_SET_BIT(SYSTEM_PERIP_CLK_EN0_REG, SYSTEM_RMT_CLK_EN);
        REG_CLR_BIT(SYSTEM_PERIP_RST_EN0_REG, SYSTEM_RMT_RST);
    }

    // configure GPIO pin registers: set GPIO_FUNCx_OUT_SEL to 81-84 and GPIO_FUNCx_OEN_SEL to 0
    uint32_t gpio_reg = GPIO_FUNC0_OUT_SEL_CFG_REG + DEFAULT_REG_SIZE * gpio;
    reg_val = REG_READ(gpio_reg);
    uint32_t signum = RTM_SIG_OUT0_NUM + rmt_channel_tx;
    // clear GPIO_FUNCx_OUT_SEL
    reg_val &= ~GPIO_MATRIX_OUT_SEL_FIELD_MASK;
    // clear GPIO_FUNCx_OEN_SEL
    reg_val &= ~GPIO_MATRIX_OEN_SEL_FIELD_MASK;
    // set signum GPIO_FUNCx_OUT_SEL
    reg_val |= signum & GPIO_MATRIX_OUT_SEL_FIELD_MASK;
    REG_WRITE(gpio_reg, reg_val);
    reg_val = REG_READ(gpio_reg);
    ESP_LOGI(TAG, "GPIO_FUNCx_OUT_SEL_CFG_REG and GPIO_FUNCx_OEN_SEL configured. Register value: 0x%" PRIX32, reg_val);

    if (gpio < 32) {
        REG_SET_BIT(GPIO_ENABLE_W1TS_REG, BIT(gpio));
    } else {
        REG_SET_BIT(GPIO_ENABLE1_W1TS_REG, BIT(gpio - 32));
    }

    reg_val = REG_READ(GPIO_ENABLE_REG);
    ESP_LOGI(TAG, "GPIO_ENABLE_REG configured. Register value: 0x%" PRIX32, reg_val);

    // configure IO_MUX_x_REG:
    // clear IO_MUX_FUN_WPU and IO_MUX_FUN_WPD to disable internal pull-up/down resistors
    uint32_t iomux_reg = IO_MUX_GPIO0_REG + DEFAULT_REG_SIZE * gpio;
    // IO_MUX_MCU_DRV[10:11] -> 2, IO_MUX_MCU_SEL to 1
    reg_val = BIT(11) | BIT(12);
    REG_WRITE(iomux_reg, reg_val);
    reg_val = REG_READ(iomux_reg);
    ESP_LOGI(TAG, "IO_MUX_GPIOn_REG configured. Register value: 0x%" PRIX32, reg_val);

    // Configure RMT_SYS_CONF_REG:
    // RMT_CLK_EN - 1
    // RMT_SCLK_SEL - 1 (APB_CLK 80MHz)
    // RMT_SCLK_DIV_NUM - 1 (default),
    // RMT_CLK div = 1 + 1 + 0/0 = 2, e.g. RMT_CLK frequency is 40MHz
    reg_val = REG_READ(RMT_SYS_CONF_REG);
    ESP_LOGI(TAG, "RMT_SYS_CONF_REG register value: 0x%" PRIX32, reg_val);
    if (!(reg_val & (1 << 31))) {
        ESP_LOGI(TAG, "RMT_SYS_CONF_REG is not configured. Configuring...");
        // set RMT_SCLK_DIV_NUM to 1
        reg_val = (reg_val & ~(0xFF << 4)) | (1 << 4);
        // set RMT_SCLK_SEL to APB_CLK
        reg_val = (reg_val & ~(0x3 << 24)) | (1 << 24);
        // set RMT_CLK_EN to 1
        reg_val |= (1 << 31);
        REG_WRITE(RMT_SYS_CONF_REG, reg_val);
        reg_val = REG_READ(RMT_SYS_CONF_REG);
        ESP_LOGI(TAG, "RMT_SYS_CONF_REG configured. Register value: 0x%" PRIX32, reg_val);
    }

    // Configure channel
    uint32_t channel_reg = RMT_CH0CONF0_REG + DEFAULT_REG_SIZE * rmt_channel_tx;
    reg_val = REG_READ(channel_reg);

    // RMT_DIV_CNT_CHn
    int channel_div = (RTM_SCLK_FREQ_MAX / resolution_hz);
    if (channel_div > 255)
        channel_div = 255;
    if (channel_div < 1)
        channel_div = 1;
    reg_val = (reg_val & ~(0xFF << 8)) | (channel_div << 8);
    // RMT_MEM_SIZE_CHn - 1 (48 * 4 байт)
    reg_val = (reg_val & ~(0xF << 16)) | (1 << 16);
    // RMT_IDLE_OUT_LV_CHn - 0
    // reg_val &= ~(1 << 5);
    reg_val |= (1 << 5);
    // RMT_IDLE_OUT_EN_CHn -1
    reg_val = (reg_val & ~(1 << 6)) | (1 << 6);

    // RMT_CARRIER_EFF_EN_CHn - 0
    // reg_val &= ~(1 << 20);
    // RMT_CARRIER_EN_CHn - 0
    reg_val &= ~(1 << 21);
    // RMT_CARRIER_OUT_LV_CHn - 0
    // reg_val &= ~(1 << 22);

    // RMT_CONF_UPDATE_CHn - 1
    reg_val |= (1 << 24);
    REG_WRITE(channel_reg, reg_val);
    reg_val = REG_READ(channel_reg);
    ESP_LOGI(TAG, "RMT_CHnCONF0_REG configured. Register value: 0x%" PRIX32, reg_val);

    return ESP_OK;
}

void rmt_raw_send_pulses(int rmt_channel_tx, const uint32_t duration_us, size_t count, int start_level) {
    uint32_t channel_reg = RMT_CH0CONF0_REG + DEFAULT_REG_SIZE * rmt_channel_tx;
    reg_val = REG_READ(channel_reg);
    // RMT_APB_MEM_RST_CHn - 1
    REG_SET_BIT(channel_reg, BIT(2));
    reg_val = REG_READ(channel_reg);
    ESP_LOGI(TAG, "RMT_CHnCONF0_REG => RMT_APB_MEM_RST_CHn done for channel %d. Register value: 0x%" PRIX32, rmt_channel_tx, reg_val);

    uint32_t reg_data = RMT_CH0DATA_REG + DEFAULT_REG_SIZE * rmt_channel_tx;

    uint32_t ticks = duration_us;
    uint32_t first_pulse = (ticks & 0x7FFF) | ((uint32_t)(start_level & 0x1) << 15);
    uint32_t second_pulse = first_pulse ^ (1 << 15);
    for (int i = 0; i < count; i++) {
        REG_WRITE(reg_data, (second_pulse << 16) | first_pulse);
    }
    REG_WRITE(reg_data, 0);

    REG_SET_BIT(RMT_INT_ENA_REG, BIT(rmt_channel_tx));
    reg_val = REG_READ(RMT_INT_ENA_REG);
    ESP_LOGI(TAG, "RMT_INT_ENA_REG value: 0x%" PRIX32, reg_val);

    // reset RMT_MEM_RD_RST_CHn and start transmition
    reg_val = REG_READ(channel_reg);
    // RMT_MEM_RD_RST_CHn - 1, RMT_TX_START_CHn -1
    reg_val |= 0x3;  // 0b11
    REG_WRITE(channel_reg, reg_val);
    reg_val = REG_READ(channel_reg);
    ESP_LOGI(TAG, "RMT_CHnCONF0_REG => RMT_MEM_RD_RST_CHn and RMT_TX_START_CHn done for channel %d. Register value: 0x%" PRIX32,
             rmt_channel_tx, reg_val);
}

void rmt_raw_wait(int rmt_channel_tx) {
    uint32_t raw;
    do {
        raw = REG_READ(RMT_INT_RAW_REG);
        ESP_LOGI(TAG, "RMT_INT_RAW_REG: 0x%08" PRIX32, raw);
        ESP_LOGI(TAG, "Waiting for transmission to finish... Sleeping 50ms");
        vTaskDelay(pdMS_TO_TICKS(10));
    } while (!(raw & BIT(rmt_channel_tx)));

    ESP_LOGI(TAG, "Transmission finished, clearing the interrupt");
    REG_WRITE(RMT_INT_RAW_REG, BIT(rmt_channel_tx));

    REG_CLR_BIT(RMT_INT_CLR_REG, BIT(rmt_channel_tx));
}

uint32_t make_pulse_word(uint32_t duration_us, uint8_t first_lvl, uint8_t second_lvl) {
    uint32_t fist_part = (duration_us & 0x7fff) | (first_lvl << 15);
    uint32_t second_part = (duration_us & 0x7fff) | (second_lvl << 15);
    return (fist_part | (second_part << 16));
}

void rmt_uart_send_byte(int rmt_channel_tx, uint8_t data, uint32_t baud_rate, uint32_t resolution_hz) {
    uint32_t bit_duration_us = resolution_hz / baud_rate;
    uint32_t channel_reg = RMT_CH0CONF0_REG + DEFAULT_REG_SIZE * rmt_channel_tx;
    uint32_t reg_data = RMT_CH0DATA_REG + DEFAULT_REG_SIZE * rmt_channel_tx;

    REG_SET_BIT(channel_reg, BIT(2));
    REG_WRITE(reg_data, make_pulse_word(bit_duration_us, 0, (data >> 0) & 1));
    REG_WRITE(reg_data, make_pulse_word(bit_duration_us, (data >> 1) & 1, (data >> 2) & 1));
    REG_WRITE(reg_data, make_pulse_word(bit_duration_us, (data >> 3) & 1, (data >> 4) & 1));
    REG_WRITE(reg_data, make_pulse_word(bit_duration_us, (data >> 5) & 1, (data >> 6) & 1));
    REG_WRITE(reg_data, make_pulse_word(bit_duration_us, (data >> 7) & 1, 1));
    REG_WRITE(reg_data, 0);
    uint32_t reg_val = REG_READ(channel_reg);
    reg_val |= 0x3;  // RMT_MEM_RD_RST_CHn і RMT_TX_START_CHn
    REG_WRITE(channel_reg, reg_val);
}

void app_main(void) {
    static const int channel = 0;
    rmt_tx_raw_init(RTM_GPIO, channel, 1000000);

    vTaskDelay(pdMS_TO_TICKS(5000));
    char* text = "Hello World!";
    for (int i = 0; i < 12; i++) {
        rmt_uart_send_byte(channel, text[i], 9600, 1000000);
        rmt_raw_wait(channel);
    }
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
