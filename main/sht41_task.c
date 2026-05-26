#include "sht41_task.h"

#include "driver/i2c_master.h"
#include "esp_check.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "i2c_master.h"

static esp_err_t eConfigure(void);
static esp_err_t eCheckCRC(uint8_t* raw_data);
static SHT41CommandDuration_t eGetCommandDuration(SHT41Command_t command);
static esp_err_t eReadTemperatureAndHumidity(float* p_temp, float* p_hum);
static void vSHT41Task(void* pvParameters);

static const char* LOG_PREFIX = "[SHT41]";

static i2c_master_dev_handle_t i2c_dev_handle;
static float temperature;
static float humidity;

void vSHT41CreateTask(void) {
    xTaskCreate(vSHT41Task, "SHT41 Task", 4096, NULL, 5, NULL);
}

static esp_err_t eConfigure(void) {
    ESP_RETURN_ON_ERROR(eI2CConfigureMaster(), LOG_PREFIX, "Failed to configure i2c interface");
    ESP_RETURN_ON_ERROR(eI2CAddDevice(SHT41_I2C_ADDRESS, &i2c_dev_handle), LOG_PREFIX, "Failed to configure i2c device");
    return ESP_OK;
}

static esp_err_t eCheckCRC(uint8_t* raw_data) {
    uint8_t crc = 0xFF;

    for (uint8_t i = 0; i < 2; i++) {
        crc ^= raw_data[i];
        for (uint8_t bit = 8; bit > 0; bit--) {
            if (crc & 0x80) {
                crc = (crc << 1) ^ 0x31;
            } else {
                crc = (crc << 1);
            }
        }
    }

    if (crc != raw_data[2]) {
        return ESP_ERR_INVALID_CRC;
    }
    return ESP_OK;
}

static SHT41CommandDuration_t eGetCommandDuration(SHT41Command_t command) {
    switch (command) {
        case SHT41_CMD_LOW_MEASUREMENT:
            return SHT41_DUR_LOW_MEASUREMENT;
        case SHT41_CMD_MED_MEASUREMENT:
            return SHT41_DUR_MED_MEASUREMENT;
        case SHT41_CMD_HIG_MEASUREMENT:
            return SHT41_DUR_HIG_MEASUREMENT;
        case SHT41_CMD_SERIAL_NUM:
            return SHT41_DUR_SERIAL_NUM;
        case SHT41_CMD_SOFT_RESET:
            return SHT41_DUR_SOFT_RESET;
        default:
            return SHT41_DUR_DEFAULT;
    }
}

static esp_err_t eReadTemperatureAndHumidity(float* p_temp, float* p_hum) {
    uint8_t command = SHT41_CMD_HIG_MEASUREMENT;
    uint8_t res_buf[6];
    uint16_t raw_temp;
    uint16_t raw_hummidity;

    ESP_RETURN_ON_ERROR(i2c_master_transmit(i2c_dev_handle, &command, sizeof(command), SHT41_I2C_MASTER_TIMEOUT_MS), LOG_PREFIX,
                        "Failed to send command: %d", command);

    vTaskDelay(pdMS_TO_TICKS(eGetCommandDuration(command)));

    ESP_RETURN_ON_ERROR(i2c_master_receive(i2c_dev_handle, res_buf, sizeof(res_buf), SHT41_I2C_MASTER_TIMEOUT_MS), LOG_PREFIX,
                        "Failed to read data for command %d", command);

    ESP_RETURN_ON_ERROR(eCheckCRC(&res_buf[0]), LOG_PREFIX, "CRC check for temperature failed!");
    ESP_RETURN_ON_ERROR(eCheckCRC(&res_buf[3]), LOG_PREFIX, "CRC check for humidity failed!");

    raw_temp = ((uint16_t)res_buf[0] << 8) | res_buf[1];
    raw_hummidity = ((uint16_t)res_buf[3] << 8) | res_buf[4];

    *p_temp = -45.0f + 175.0f * (float)raw_temp / ((float)(1 << 16) - 1);
    *p_hum = -6.0f + 125.0f * (float)raw_hummidity / ((float)(1 << 16) - 1);
    return ESP_OK;
}

static void vSHT41Task(void* pvParameters) {
    if (eConfigure() != ESP_OK) {
        ESP_LOGE(LOG_PREFIX, "Fatal: I2C initialization failed. Deleting task.");
        vTaskDelete(NULL);
    }

    while (1) {
        if (eReadTemperatureAndHumidity(&temperature, &humidity) == ESP_OK) {
            ESP_LOGI(LOG_PREFIX, "Temperature: %.2f °C, Humidity: %.2f %%", temperature, humidity);
        } else {
            ESP_LOGE(LOG_PREFIX, "Failed to read sensors on this turn");
        }
        vTaskDelay(pdMS_TO_TICKS(SHT41_TASK_SLEEP_MS));
    }
}
