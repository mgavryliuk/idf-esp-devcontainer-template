#include "i2c_master.h"

#include "driver/i2c_master.h"
#include "esp_check.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

static const char* LOG_PREFIX = "[I2C Master]";
static SemaphoreHandle_t xI2CMutex = NULL;
static bool bIsConfigured = false;

i2c_master_bus_handle_t i2c_bus_handle = NULL;

esp_err_t eI2CConfigureMaster(void) {
    if (bIsConfigured) {
        ESP_LOGD(LOG_PREFIX, "Master already configured!");
        return ESP_OK;
    }

    if (xI2CMutex == NULL)
        xI2CMutex = xSemaphoreCreateMutex();

    if (xSemaphoreTake(xI2CMutex, portMAX_DELAY) == pdTRUE) {
        if (bIsConfigured) {
            ESP_LOGD(LOG_PREFIX, "Master already configured!");
            xSemaphoreGive(xI2CMutex);
            return ESP_OK;
        }

        i2c_master_bus_config_t i2c_bus_config = {
            .i2c_port = I2C_MASTER_NUM,
            .sda_io_num = I2C_MASTER_SDA_IO,
            .scl_io_num = I2C_MASTER_SCL_IO,
            .clk_source = I2C_CLK_SRC_DEFAULT,
            .glitch_ignore_cnt = 7,
            .flags.enable_internal_pullup = true,
        };
        esp_err_t ret = i2c_new_master_bus(&i2c_bus_config, &i2c_bus_handle);
        if (ret == ESP_OK) {
            bIsConfigured = true;
        }

        xSemaphoreGive(xI2CMutex);
        return ret;
    }

    return ESP_OK;
}

esp_err_t eI2CAddDevice(uint16_t addr, i2c_master_dev_handle_t* i2c_dev_handle) {
    if (!bIsConfigured || i2c_bus_handle == NULL) {
        ESP_LOGE(LOG_PREFIX, "I2C Bus is not configured yet!");
        return ESP_ERR_INVALID_STATE;
    }

    i2c_device_config_t i2c_device_config = {
        .dev_addr_length = I2C_ADDR_BIT_7,
        .device_address = addr,
        .scl_speed_hz = I2C_MASTER_FREQ_HZ,
    };
    ESP_RETURN_ON_ERROR(i2c_master_bus_add_device(i2c_bus_handle, &i2c_device_config, i2c_dev_handle), LOG_PREFIX,
                        "Failed to init I2C device with address: %d", addr);
    return ESP_OK;
}
