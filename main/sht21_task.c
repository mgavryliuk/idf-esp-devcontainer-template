#include "sht21_task.h"

#include "driver/i2c_master.h"
#include "esp_check.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static esp_err_t eConfigure(void);
static esp_err_t eCheckCRC(uint8_t* raw_data);
static esp_err_t eReadValue(uint16_t* dest_raw, SHT21Commant_t command, uint8_t measurement_delay);
static esp_err_t eReadTemperature(float* p_temp);
static esp_err_t eReadHumidity(float* p_humidity);
static void vSHT21Task(void* pvParameters);

static const char* LOG_PREFIX = "[SHT21]";

static i2c_master_bus_handle_t i2c_bus_handle;
static i2c_master_dev_handle_t i2c_dev_handle;
static float temperature;
static float humidity;

void vSHT21CreateTask(void) {
    xTaskCreate(vSHT21Task, "SHT21 Task", 4096, NULL, 5, NULL);
}

static esp_err_t eConfigure(void) {
    i2c_master_bus_config_t i2c_bus_config = {
        .i2c_port = SHT21_I2C_MASTER_NUM,
        .sda_io_num = SHT21_MASTER_SDA_IO,
        .scl_io_num = SHT21_MASTER_SCL_IO,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };
    ESP_RETURN_ON_ERROR(i2c_new_master_bus(&i2c_bus_config, &i2c_bus_handle), LOG_PREFIX, "Failed to init I2C bus");

    i2c_device_config_t i2c_device_config = {
        .dev_addr_length = I2C_ADDR_BIT_7,
        .device_address = SHT21_I2C_ADDRESS,
        .scl_speed_hz = SHT21_I2C_MASTER_FREQ_HZ,
    };
    ESP_RETURN_ON_ERROR(i2c_master_bus_add_device(i2c_bus_handle, &i2c_device_config, &i2c_dev_handle), LOG_PREFIX,
                        "Failed to init I2C device");
    return ESP_OK;
}

static esp_err_t eCheckCRC(uint8_t* raw_data) {
    uint8_t crc = 0;

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

static esp_err_t eReadValue(uint16_t* dest_raw, SHT21Commant_t command, uint8_t measurement_delay) {
    uint8_t write_buf = command;
    ESP_RETURN_ON_ERROR(i2c_master_transmit(i2c_dev_handle, &write_buf, sizeof(write_buf), SHT21_I2C_MASTER_TIMEOUT_MS), LOG_PREFIX,
                        "Failed to send command: %d", command);

    vTaskDelay(pdMS_TO_TICKS(measurement_delay));

    uint8_t res_buf[3];
    ESP_RETURN_ON_ERROR(i2c_master_receive(i2c_dev_handle, res_buf, sizeof(res_buf), SHT21_I2C_MASTER_TIMEOUT_MS), LOG_PREFIX,
                        "Failed to read data for command %d", command);

    ESP_RETURN_ON_ERROR(eCheckCRC(res_buf), LOG_PREFIX, "CRC check failed!");
    *dest_raw = ((uint16_t)res_buf[0] << 8) | (res_buf[1] & 0xFC);
    return ESP_OK;
}

static esp_err_t eReadTemperature(float* p_temp) {
    uint16_t raw_reading;
    ESP_RETURN_ON_ERROR(eReadValue(&raw_reading, SHT21_CMD_TRIG_T_MEASUREMENT_NHM, SHT21_DELAY_T_MEASUREMENT), LOG_PREFIX,
                        "Failed to read Temperature");
    *p_temp = -46.85f + 175.72f * (float)raw_reading / (float)(1 << 16);
    return ESP_OK;
}

static esp_err_t eReadHumidity(float* p_humidity) {
    uint16_t raw_reading;
    ESP_RETURN_ON_ERROR(eReadValue(&raw_reading, SHT21_CMD_TRIG_RH_MEASUREMENT_NHM, SHT21_DELAY_RH_MEASUREMENT), LOG_PREFIX,
                        "Failed to read Humidity");
    *p_humidity = -6.0f + 125.0f * (float)raw_reading / (float)(1 << 16);
    return ESP_OK;
}

static void vSHT21Task(void* pvParameters) {
    if (eConfigure() != ESP_OK) {
        ESP_LOGE(LOG_PREFIX, "Fatal: I2C initialization failed. Deleting task.");
        vTaskDelete(NULL);
    }

    while (1) {
        if (eReadTemperature(&temperature) == ESP_OK && eReadHumidity(&humidity) == ESP_OK) {
            ESP_LOGI(LOG_PREFIX, "Temperature: %.2f °C, Humidity: %.2f %%", temperature, humidity);
        } else {
            ESP_LOGE(LOG_PREFIX, "Failed to read sensors on this turn");
        }
        vTaskDelay(pdMS_TO_TICKS(SHT21_TASK_SLEEP_MS));
    }
}
