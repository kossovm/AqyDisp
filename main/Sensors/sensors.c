#include "sensors.h"

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <inttypes.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_system.h>
#include <esp_log.h>
#include <esp_err.h>

#include <bmp280.h>
#include <ina219.h>
#include <scd4x.h>
#include <aht.h>
#include <dht.h>

#include <ds18x20.h>

#ifndef APP_CPU_NUM
#define APP_CPU_NUM PRO_CPU_NUM
#endif

#define I2C_PORT 1


//////////////////INA219 CONFIGS////////////////////////

#define INA219_ADDR_DEFAULT INA219_ADDR_GND_GND
#define CONFIG_EXAMPLE_SHUNT_RESISTOR_MILLI_OHM 100

//////////////////////////////////////////////////////




/////////////////DS18X20 CONFIGS///////////////////////

#define DS18X20_ADDR_DEFAULT DS18X20_ANY

#define DS18X20_GPIO_DEFAULT -1

//////////////////////////////////////////////////////





///////////////AHT10 CONFIGS//////////////////////////

#ifdef CONFIG_EXAMPLE_I2C_ADDRESS_GND
#define ADDR AHT_I2C_ADDRESS_GND
#endif
#ifdef CONFIG_EXAMPLE_I2C_ADDRESS_VCC
#define ADDR AHT_I2C_ADDRESS_VCC
#endif

#ifdef CONFIG_EXAMPLE_TYPE_AHT1x
#define AHT_TYPE AHT_TYPE_AHT1x
#endif

#ifdef CONFIG_EXAMPLE_TYPE_AHT20
#define AHT_TYPE AHT_TYPE_AHT20
#endif

//////////////////////////////////////////////////////



///////////////////DHT CONFIGS/////////////////////////

#ifdef CONFIG_EXAMPLE_TYPE_DHT11
#define SENSOR_TYPE DHT_TYPE_DHT11
#endif
#ifdef CONFIG_EXAMPLE_TYPE_AM2301
#define SENSOR_TYPE DHT_TYPE_AM2301
#endif
#ifdef CONFIG_EXAMPLE_TYPE_SI7021
#define SENSOR_TYPE DHT_TYPE_SI7021
#endif

///////////////////////////////////////////////////////




static const char *TAG = "AqyLab_Sensors";

extern i2c_master_bus_handle_t i2c_bus_handle;

esp_err_t i2c_master_bus_init(i2c_master_bus_handle_t *bus_handle)
{
    const i2c_master_bus_config_t i2c_bus_config = {
        .i2c_port = EXAMPLE_I2C_PORT_NUM,
        .sda_io_num = EXAMPLE_SDA,
        .scl_io_num = EXAMPLE_SCL,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };
    
    ESP_ERROR_CHECK(i2c_new_master_bus(&i2c_bus_config, bus_handle));

    return ESP_OK;
}

// esp_err_t i2c_deinit(void)
// {
//     ESP_RETURN_ON_ERROR(i2c_driver_delete(EXAMPLE_TOUCH_I2C_NUM), TAG, "I2C DE-initialization failed");
//     return ESP_OK;
// }


esp_err_t i2c_master_device_init(i2c_master_bus_handle_t *i2c_bus_handle, i2c_master_dev_handle_t *i2c_device_handle, uint16_t i2c_device_address, uint32_t i2c_device_frequency) {


    i2c_device_config_t i2c_device_config = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = i2c_device_address,
        .scl_speed_hz = i2c_device_frequency,
    };

    ESP_ERROR_CHECK(i2c_master_bus_add_device(*i2c_bus_handle, &i2c_device_config, i2c_device_handle));

    return ESP_OK;
}


void esp_idf_lib_i2cdev_init() {
    ESP_ERROR_CHECK(i2cdev_init());
}



//////////////I2C SENSORS//////////////////

void bmp280_test(void *pvParameters)
{
    bmp280_params_t params;
    bmp280_init_default_params(&params);
    bmp280_t dev;
    memset(&dev, 0, sizeof(bmp280_t));

    ESP_ERROR_CHECK(bmp280_init_desc(&dev, BMP280_I2C_ADDRESS_0, (i2c_port_t)I2C_PORT, (gpio_num_t)EXAMPLE_SDA, (gpio_num_t)EXAMPLE_SCL));
    ESP_ERROR_CHECK(bmp280_init(&dev, &params));

    bool bme280p = dev.id == BME280_CHIP_ID;
    printf("BMP280: found %s\n", bme280p ? "BME280" : "BMP280");

    float pressure, temperature, humidity;

    while (1)
    {
        vTaskDelay(pdMS_TO_TICKS(500));
        if (bmp280_read_float(&dev, &temperature, &pressure, &humidity) != ESP_OK)
        {
            printf("Temperature/pressure reading failed\n");
            continue;
        }

        /* float is used in printf(). you need non-default configuration in
         * sdkconfig for ESP8266, which is enabled by default for this
         * example. see sdkconfig.defaults.esp8266
         */
        printf("Pressure: %.2f Pa, Temperature: %.2f C", pressure, temperature);
        if (bme280p)
            printf(", Humidity: %.2f\n", humidity);
        else
            printf("\n");
    }
}


void ina219_test(void *pvParameters)
{
    ina219_t dev;
    memset(&dev, 0, sizeof(ina219_t));

    assert(CONFIG_EXAMPLE_SHUNT_RESISTOR_MILLI_OHM > 0);
    ESP_ERROR_CHECK(ina219_init_desc(&dev, INA219_ADDR_DEFAULT, (i2c_port_t)I2C_PORT, (gpio_num_t)EXAMPLE_SDA, (gpio_num_t)EXAMPLE_SCL));
    ESP_LOGI(TAG, "Initializing INA219");
    ESP_ERROR_CHECK(ina219_init(&dev));

    ESP_LOGI(TAG, "Configuring INA219");
    ESP_ERROR_CHECK(ina219_configure(&dev, INA219_BUS_RANGE_16V, INA219_GAIN_0_125,
            INA219_RES_12BIT_1S, INA219_RES_12BIT_1S, INA219_MODE_CONT_SHUNT_BUS));

    ESP_LOGI(TAG, "Calibrating INA219");

    ESP_ERROR_CHECK(ina219_calibrate(&dev, (float)CONFIG_EXAMPLE_SHUNT_RESISTOR_MILLI_OHM / 1000.0f));

    float bus_voltage, shunt_voltage, current, power;

    ESP_LOGI(TAG, "Starting the loop");
    while (1)
    {
        ESP_ERROR_CHECK(ina219_get_bus_voltage(&dev, &bus_voltage));
        ESP_ERROR_CHECK(ina219_get_shunt_voltage(&dev, &shunt_voltage));
        ESP_ERROR_CHECK(ina219_get_current(&dev, &current));
        ESP_ERROR_CHECK(ina219_get_power(&dev, &power));
        /* Using float in printf() requires non-default configuration in
         * sdkconfig. See sdkconfig.defaults.esp32 and
         * sdkconfig.defaults.esp8266  */
        printf("VBUS: %.04f V, VSHUNT: %.04f mV, IBUS: %.04f mA, PBUS: %.04f mW\n",
                bus_voltage, shunt_voltage * 1000, current * 1000, power * 1000);

        vTaskDelay(pdMS_TO_TICKS(500));
    }
}


void scd41_test(void *pvParameters)
{
    i2c_dev_t dev = { 0 };

    ESP_ERROR_CHECK(scd4x_init_desc(&dev, (i2c_port_t)I2C_PORT, (gpio_num_t)EXAMPLE_SDA, (gpio_num_t)EXAMPLE_SCL));

    ESP_LOGI(TAG, "Initializing sensor...");
    ESP_ERROR_CHECK(scd4x_wake_up(&dev));
    ESP_ERROR_CHECK(scd4x_stop_periodic_measurement(&dev));
    ESP_ERROR_CHECK(scd4x_reinit(&dev));
    ESP_LOGI(TAG, "Sensor initialized");

    uint16_t serial[3];
    ESP_ERROR_CHECK(scd4x_get_serial_number(&dev, serial, serial + 1, serial + 2));
    ESP_LOGI(TAG, "Sensor serial number: 0x%04x%04x%04x", serial[0], serial[1], serial[2]);

    ESP_ERROR_CHECK(scd4x_start_periodic_measurement(&dev));
    ESP_LOGI(TAG, "Periodic measurements started");

    uint16_t co2;
    float temperature, humidity;
    while (1)
    {
        vTaskDelay(pdMS_TO_TICKS(5000));

        esp_err_t res = scd4x_read_measurement(&dev, &co2, &temperature, &humidity);
        if (res != ESP_OK)
        {
            ESP_LOGE(TAG, "Error reading results %d (%s)", res, esp_err_to_name(res));
            continue;
        }

        if (co2 == 0)
        {
            ESP_LOGW(TAG, "Invalid sample detected, skipping");
            continue;
        }

        ESP_LOGI(TAG, "CO2: %u ppm", co2);
        ESP_LOGI(TAG, "Temperature: %.2f °C", temperature);
        ESP_LOGI(TAG, "Humidity: %.2f %%", humidity);
    }
}


void aht10_task(void *pvParameters)
{
    aht_t dev = { 0 };
    dev.mode = AHT_MODE_NORMAL;
    dev.type = AHT_TYPE;

    ESP_ERROR_CHECK(aht_init_desc(&dev, ADDR, (i2c_port_t)I2C_PORT, (gpio_num_t)EXAMPLE_SDA, (gpio_num_t)EXAMPLE_SCL));
    ESP_ERROR_CHECK(aht_init(&dev));

    bool calibrated;
    ESP_ERROR_CHECK(aht_get_status(&dev, NULL, &calibrated));
    if (calibrated)
        ESP_LOGI(TAG, "Sensor calibrated");
    else
        ESP_LOGW(TAG, "Sensor not calibrated!");

    float temperature, humidity;

    while (1)
    {
        esp_err_t res = aht_get_data(&dev, &temperature, &humidity);
        if (res == ESP_OK)
            ESP_LOGI(TAG, "Temperature: %.1f°C, Humidity: %.2f%%", temperature, humidity);
        else
            ESP_LOGE(TAG, "Error reading data: %d (%s)", res, esp_err_to_name(res));

        vTaskDelay(pdMS_TO_TICKS(500));
    }
}




////////////ONE WIRE SENSORS////////////////


void ds18x20_test(void *pvParameter)
{
    // Make sure that the internal pull-up resistor is enabled on the GPIO pin
    // so that one can connect up a sensor without needing an external pull-up.
    // (Note: The internal (~47k) pull-ups of the ESP do appear to work, at
    // least for simple setups (one or two sensors connected with short leads),
    // but do not technically meet the pull-up requirements from the ds18x20
    // datasheet and may not always be reliable. For a real application, a proper
    // 4.7k external pull-up resistor is recommended instead!)

    gpio_set_pull_mode(DS18X20_GPIO_DEFAULT, GPIO_PULLUP_ONLY);

    float temperature;
    esp_err_t res;

    while (1)
    {
        res = ds18x20_measure_and_read(DS18X20_GPIO_DEFAULT, DS18X20_ADDR_DEFAULT, &temperature);
        if (res != ESP_OK)
            ESP_LOGE(TAG, "Could not read from sensor %08" PRIx32 "%08" PRIx32 ": %d (%s)",
                    (uint32_t)(DS18X20_ADDR_DEFAULT >> 32), (uint32_t)DS18X20_ADDR_DEFAULT, res, esp_err_to_name(res));
        else
            ESP_LOGI(TAG, "Sensor %08" PRIx32 "%08" PRIx32 ": %.2f°C",
                    (uint32_t)(DS18X20_ADDR_DEFAULT >> 32), (uint32_t)DS18X20_ADDR_DEFAULT, temperature);

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}



//////////CUSTOM PROTOCOL SENSORS/////////////

void dht11_test(void *pvParameters)
{
    float temperature, humidity;

#ifdef CONFIG_EXAMPLE_INTERNAL_PULLUP
    gpio_set_pull_mode(CONFIG_EXAMPLE_DATA_GPIO, GPIO_PULLUP_ONLY);
#endif

    while (1)
    {
        if (dht_read_float_data(SENSOR_TYPE, CONFIG_EXAMPLE_DATA_GPIO, &humidity, &temperature) == ESP_OK)
            printf("Humidity: %.1f%% Temp: %.1fC\n", humidity, temperature);
        else
            printf("Could not read data from sensor\n");

        // If you read the sensor data too often, it will heat up
        // http://www.kandrsmith.org/RJS/Misc/Hygrometers/dht_sht_how_fast.html
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}



///////////ANALOGUE SENSORS/////////////////





void startSensorTask(TaskFunction_t sensorTask, const char * const sensorTaskName) {
    xTaskCreatePinnedToCore(sensorTask, sensorTaskName, configMINIMAL_STACK_SIZE * 8, NULL, 5, NULL, APP_CPU_NUM);
}


