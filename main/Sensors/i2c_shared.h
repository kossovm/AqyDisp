#ifndef I2C_SHARED_H
#define I2C_SHARED_H

#include "driver/i2c_master.h"
#include "i2cdev.h"

// Declare the shared bus handle (defined in one .c file)
extern i2c_master_bus_handle_t i2c_bus_handle;
extern i2c_master_dev_handle_t i2c_device_handle;

// Optionally declare the dummy device if other files need it
extern i2c_dev_t dummy_bus_holder;

#endif // I2C_SHARED_H