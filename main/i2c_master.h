#ifndef I2C_MASTER_H
#define I2C_MASTER_H

#include "driver/i2c.h"
#include "driver/i2c_master.h"
#include "esp_err.h"

#define I2C_MASTER_NUM I2C_NUM_0
#define I2C_MASTER_FREQ_HZ (100000)
#define I2C_MASTER_SCL_IO (9)
#define I2C_MASTER_SDA_IO (8)
#define I2C_MASTER_TX_BUF_DISABLE (0)
#define I2C_MASTER_RX_BUF_DISABLE (0)
#define I2C_MASTER_TIMEOUT_MS (200)

esp_err_t eI2CConfigureMaster(void);
esp_err_t eI2CAddDevice(uint16_t addr, i2c_master_dev_handle_t* i2c_dev_handle);

#endif
