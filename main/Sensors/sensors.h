#include "driver/i2c_master.h"

/* --- I2C Support --- */
#define EXAMPLE_I2C_PORT_NUM   1          // I2C port to use
#define EXAMPLE_SDA       42
#define EXAMPLE_SCL       41

extern esp_err_t i2c_master_bus_init(i2c_master_bus_handle_t *i2c_bus_handle);
extern esp_err_t i2c_master_device_init(i2c_master_bus_handle_t *i2c_bus_handle, i2c_master_dev_handle_t *device_handle, uint16_t device_address, uint32_t device_frequency);