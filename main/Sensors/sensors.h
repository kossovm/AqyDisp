#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <inttypes.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>


#include "driver/i2c_master.h"

#include "i2cdev.h"




/* --- I2C Support --- */
#define EXAMPLE_I2C_PORT_NUM   1          // I2C port to use
#define EXAMPLE_SDA       42
#define EXAMPLE_SCL       41

extern esp_err_t i2c_master_bus_init(i2c_master_bus_handle_t *i2c_bus_handle);
extern esp_err_t i2c_master_device_init(i2c_master_bus_handle_t *i2c_bus_handle, i2c_master_dev_handle_t *device_handle, uint16_t device_address, uint32_t device_frequency);




void esp_idf_lib_i2cdev_init();

void initiateDummyDeviceForBUUUUS();

void startSensorTask(TaskFunction_t sensorTask, const char * const sensorTaskName);

void bmp280_test(void *pvParameters);
void ina219_test(void *pvParameters);

void scd41_dummy_init();
void scd41_test(void *pvParameters);

void aht10_test(void *pvParameters);

void ds18x20_test(void *pvParameter);

void dht11_test(void *pvParameters);

void mq8_print_values_test(void *pvParameters);