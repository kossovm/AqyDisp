#include "sensors.h"

#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include <esp_system.h>
#include <esp_log.h>
#include <esp_err.h>
#include "soc/soc_caps.h"
#include <math.h>

#include <bmp280.h>
#include <ina219.h>
#include <scd4x.h>
#include <aht.h>
#include <dht.h>

#include <ds18x20.h>

#include "GUI/ui.h"
#include "GUI/screens/ui_graphScreen.h"
#include "esp_lvgl_port.h"

#include "i2c_shared.h"

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

#define DS18X20_GPIO_DEFAULT 6

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






//////////////////ANALOGUE CONFIGS/////////////////////

#define EXAMPLE_ADC1_CHAN5_GPIO6          ADC_CHANNEL_5

#define EXAMPLE_ADC_ATTEN           ADC_ATTEN_DB_12



///////////////////////////////////////////////////////


////////////DUMMY CONFIG////////////////////////////

#define DUMMY_ADDR    0x00




static const char *TAG = "AqyLab_Sensors";





/////////////////DUMMY + Unused Testing Functions/////////////////////

i2c_dev_t dummy_bus_holder = {0};
i2c_master_bus_handle_t i2c_bus_handle = NULL;

void initiateDummyDeviceForBUUUUS() {
    memset(&dummy_bus_holder, 0, sizeof(i2c_dev_t));

    dummy_bus_holder.port = I2C_PORT;
    dummy_bus_holder.addr = DUMMY_ADDR;
    dummy_bus_holder.cfg.sda_io_num = EXAMPLE_SDA;
    dummy_bus_holder.cfg.scl_io_num = EXAMPLE_SCL;
    dummy_bus_holder.cfg.master.clk_speed = 400000;


    /* 3. Allocate bus + mutex without putting bytes on the wire */
    ESP_ERROR_CHECK(i2c_dev_create_mutex(&dummy_bus_holder));

    esp_err_t probe_err = i2c_dev_check_present(&dummy_bus_holder);
    if (probe_err != ESP_OK) {
        ESP_LOGW(TAG, "Dummy device probe failed (expected): %s", esp_err_to_name(probe_err));
    }

    // Now get the bus handle using the new function
    i2c_bus_handle = i2cdev_get_bus_handle(I2C_PORT);
    
    if (i2c_bus_handle == NULL) {
        ESP_LOGE(TAG, "Failed to get I2C bus handle!");
    } else {
        ESP_LOGI(TAG, "I2C bus handle acquired: %p", (void*)i2c_bus_handle);
    }
}







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

void scd41_dummy_init() {
    i2c_dev_t dev = { 0 };

    ESP_ERROR_CHECK(scd4x_init_desc(&dev, (i2c_port_t)I2C_PORT, (gpio_num_t)EXAMPLE_SDA, (gpio_num_t)EXAMPLE_SCL));

    // ESP_LOGI(TAG, "Initializing sensor...");
    // //ESP_ERROR_CHECK(scd4x_wake_up(&dev));
    ESP_ERROR_CHECK(scd4x_stop_periodic_measurement(&dev));
    
    ESP_ERROR_CHECK(scd4x_reinit(&dev));

    ESP_LOGI(TAG, "Dummy initialized");
}

void scd41_test(void *pvParameters)
{
    
    i2c_dev_t dev = { 0 };

    ESP_ERROR_CHECK(scd4x_init_desc(&dev, (i2c_port_t)I2C_PORT, (gpio_num_t)EXAMPLE_SDA, (gpio_num_t)EXAMPLE_SCL));

    ESP_LOGI(TAG, "Initializing sensor...");
    //ESP_ERROR_CHECK(scd4x_wake_up(&dev));
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

    lvgl_port_lock(0);
    lv_chart_series_t *co2ReadingsDataSeries = lv_chart_add_series(uic_Chart1, lv_color_hex(0x6C42C1), LV_CHART_AXIS_PRIMARY_Y);
    lv_chart_set_x_start_point(uic_Chart1, co2ReadingsDataSeries, 0);
    lvgl_port_unlock();

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

        ESP_LOGI(TAG, "YEYA");

        lvgl_port_lock(0);

        lv_chart_set_range(uic_Chart1, LV_CHART_AXIS_PRIMARY_Y, 0, co2 + 300);
    
        lv_chart_set_next_value(uic_Chart1, co2ReadingsDataSeries, (int32_t)co2);

        //lv_chart_set_cursor_point();
        lvgl_port_unlock();

    }


}


void aht10_test(void *pvParameters)
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
        
        if (lvgl_port_lock(0)) {
        
            if (mq8_series != NULL && ui_theChart != NULL) {
                // Add the new point
                lv_chart_set_next_value(ui_theChart, mq8_series, (int)temperature);
                
                // Force redraw
                lv_chart_refresh(ui_theChart);
            }
            
            // CRITICAL: You must unlock!
            lvgl_port_unlock();
        }

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



///////////ANALOGUE SENSORS boogaloo (the electric kind)/////////////////



// --- MQ-8 & Circuit Math ---
// Voltage Divider: 5V output -> Resistors -> ESP32
// Example: R1 (Series) = 2200 Ohm, R2 (GND) = 3300 Ohm
// Multiplier = (R1+R2)/R2 = (2200+3300)/3300 = 1.666
#define VOLTAGE_DIVIDER_RATIO  1.666f  

#define RL_VALUE_K      10.0f    // Load Resistor on MQ board
#define V_CIRCUIT       5.0f     // 5V VCC
#define R0_CLEAN_AIR    4.85f    // Calibrate this! (Value of Rs in clean air / 70)
#define MQ8_A           1080.0f  // Coefficient A
#define MQ8_B           -0.642f  // Coefficient B


/*---------------------------------------------------------------
        ADC Calibration
---------------------------------------------------------------*/
static bool example_adc_calibration_init(adc_unit_t unit, adc_channel_t channel, adc_atten_t atten, adc_cali_handle_t *out_handle)
{
    adc_cali_handle_t handle = NULL;
    esp_err_t ret = ESP_FAIL;
    bool calibrated = false;

#if ADC_CALI_SCHEME_CURVE_FITTING_SUPPORTED
    if (!calibrated) {
        ESP_LOGI(TAG, "calibration scheme version is %s", "Curve Fitting");
        adc_cali_curve_fitting_config_t cali_config = {
            .unit_id = unit,
            .chan = channel,
            .atten = atten,
            .bitwidth = ADC_BITWIDTH_DEFAULT,
        };
        ret = adc_cali_create_scheme_curve_fitting(&cali_config, &handle);
        if (ret == ESP_OK) {
            calibrated = true;
        }
    }
#endif

#if ADC_CALI_SCHEME_LINE_FITTING_SUPPORTED
    if (!calibrated) {
        ESP_LOGI(TAG, "calibration scheme version is %s", "Line Fitting");
        adc_cali_line_fitting_config_t cali_config = {
            .unit_id = unit,
            .atten = atten,
            .bitwidth = ADC_BITWIDTH_DEFAULT,
        };
        ret = adc_cali_create_scheme_line_fitting(&cali_config, &handle);
        if (ret == ESP_OK) {
            calibrated = true;
        }
    }
#endif

    *out_handle = handle;
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "Calibration Success");
    } else if (ret == ESP_ERR_NOT_SUPPORTED || !calibrated) {
        ESP_LOGW(TAG, "eFuse not burnt, skip software calibration");
    } else {
        ESP_LOGE(TAG, "Invalid arg or no memory");
    }

    return calibrated;
}



// Define the Semaphore for thread safety (Must be global)
SemaphoreHandle_t xGuiSemaphore = NULL;

// Define a handle for the "Line" on the chart

esp_err_t adc_config_cali_init(int *adc_raw, int *voltage) {

    xGuiSemaphore = xSemaphoreCreateMutex();

    //-------------ADC1 Init---------------//
    adc_oneshot_unit_handle_t adc_handle;
    adc_oneshot_unit_init_cfg_t init_config = {
        .unit_id = ADC_UNIT_1,
        .ulp_mode = ADC_ULP_MODE_DISABLE
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config, &adc_handle));


    //-------------ADC1 Config---------------//

    adc_oneshot_chan_cfg_t channel_config = {
        .atten = EXAMPLE_ADC_ATTEN,
        .bitwidth = ADC_BITWIDTH_DEFAULT,
    };

    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc_handle, EXAMPLE_ADC1_CHAN5_GPIO6, &channel_config));

    //-------------ADC1 Calibration Init---------------//

    adc_cali_handle_t adc_cali_chan5_handle = NULL;

    bool do_calibration1_chan0 = example_adc_calibration_init(ADC_UNIT_1, EXAMPLE_ADC1_CHAN5_GPIO6, EXAMPLE_ADC_ATTEN, &adc_cali_chan5_handle);

    static float smoothed_raw = 0.0f;
    bool first_run = true;

    while (1) {

        uint32_t adc_accumulated = 0;
        int num_samples = 64;

        for (int i = 0; i < num_samples; i++) {
            int raw_reading = 0;

            ESP_ERROR_CHECK(adc_oneshot_read(adc_handle, EXAMPLE_ADC1_CHAN5_GPIO6, &raw_reading));

            adc_accumulated += raw_reading;

            vTaskDelay(pdTICKS_TO_MS(1));
        }

        *adc_raw = adc_accumulated / num_samples;

        ESP_LOGI(TAG, "ADC%d Channel[%d] Raw Data: %d", ADC_UNIT_1 + 1, EXAMPLE_ADC1_CHAN5_GPIO6, *adc_raw);

        if (do_calibration1_chan0) {
            ESP_ERROR_CHECK(adc_cali_raw_to_voltage(adc_cali_chan5_handle, *adc_raw, voltage));
            ESP_LOGI(TAG, "ADC%d Channel[%d] Cali Voltage: %d mV", ADC_UNIT_1 + 1, EXAMPLE_ADC1_CHAN5_GPIO6, *voltage);
        }
        else {
            *voltage = *adc_raw * 3300 / 4095;
        }

        // --- MATH SECTION ---

        // 1. Convert mV to Volts (Pin Voltage)
        float v_pin = (float)*voltage / 1000.0f;

        // 2. Account for Voltage Divider (Get actual Sensor Output)
        float v_sensor = v_pin * VOLTAGE_DIVIDER_RATIO;

        // 3. Safety check to prevent divide by zero
        if (v_sensor <= 0.1 || v_sensor >= V_CIRCUIT - 0.1) {
            ESP_LOGI(TAG, "ERROR"); // Error
        }

        // 4. Calculate Sensor Resistance (Rs)
        // Rs = ((Vc - Vout) / Vout) * RL
        float rs = ((V_CIRCUIT - v_sensor) / v_sensor) * RL_VALUE_K;

        // 5. Calculate PPM using datasheet curve
        float ratio = rs / R0_CLEAN_AIR;
        float ppm = MQ8_A * pow(ratio, MQ8_B);
        
        const float BACKGROUND_NOISE_PPM = 70.6f;
        
        float real_ppm = ppm;

        // If subtraction makes it negative, clamp to 0.0
        if (real_ppm < 0) {
            real_ppm = 0.0f;
        }
    
        ESP_LOGI(TAG, "ADC%d Channel[%d] Cali PPM: %f ppm", ADC_UNIT_1 + 1, EXAMPLE_ADC1_CHAN5_GPIO6, ppm);
        ESP_LOGI(TAG, "ADC%d Channel[%d] Cali PPM: %f ppm", ADC_UNIT_1 + 1, EXAMPLE_ADC1_CHAN5_GPIO6, real_ppm);
        ESP_LOGI(TAG, "ADC%d Channel[%d] Cali RS: %f Ohm", ADC_UNIT_1 + 1, EXAMPLE_ADC1_CHAN5_GPIO6, rs);
        
        if (lvgl_port_lock(0)) {
        
            if (mq8_series != NULL && ui_theChart != NULL) {
                // Add the new point
                lv_chart_set_next_value(ui_theChart, mq8_series, (int)real_ppm);
                
                // Force redraw
                lv_chart_refresh(ui_theChart);
            }
            
            // CRITICAL: You must unlock!
            lvgl_port_unlock();
        }

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

    //     // --- 1. READ RAW ADC (Burst Average) ---
    //     uint32_t batch_sum = 0;
    //     int samples = 64;
    //     for (int i = 0; i < samples; i++) {
    //         int r;
    //         ESP_ERROR_CHECK(adc_oneshot_read(adc_handle, EXAMPLE_ADC1_CHAN5_GPIO6, &r));
    //         batch_sum += r;
    //         esp_rom_delay_us(50);
    //     }
    //     int current_raw = batch_sum / samples;

    //     // --- 2. SOFTWARE FILTER (The Magic Fix) ---
    //     if (first_run) {
    //         smoothed_raw = (float)current_raw; // Start at current value
    //         first_run = false;
    //     } else {
    //         // Strong Smoothing: 95% History, 5% New Data
    //         // If the data jumps from 1000 to 2000, this will only move to 1050.
    //         smoothed_raw = (smoothed_raw * 0.95f) + ((float)current_raw * 0.05f);
    //     }

    //     // --- 3. BASELINE SUBTRACTION (Fake Zero) ---
    //     // Look at your logs. If your sensor sits around 1000 in clean air, 
    //     // set this number to 1000.
    //     const float BASELINE_RAW = 1000.0f; 
        
    //     float display_value = smoothed_raw - BASELINE_RAW;
    //     if (display_value < 0) display_value = 0;

    //     // --- 4. LOGGING ---
    //     // We log both so you can see the filter working
    //     ESP_LOGI(TAG, "Noisy Raw: %d | Smoothed: %.1f | Graph Value: %.1f", 
    //              current_raw, smoothed_raw, display_value);

    //     // --- 5. CHART UPDATE ---
    //     if (lvgl_port_lock(0)) {
    //         if (mq8_series != NULL && ui_theChart != NULL) {
    //             // We plot the smoothed relative value
    //             lv_chart_set_next_value(ui_theChart, mq8_series, (int)display_value);
    //             lv_chart_refresh(ui_theChart);
    //         }
    //         lvgl_port_unlock();
    //     }



//NEEEDS TESTING. ARA ARA//

void mq8_print_values_test(void *pvParameters) {
    int adc_raw_m8, voltage_m8;

    adc_config_cali_init(&adc_raw_m8, &voltage_m8);
}


































////////////////////////////////////////////////////////




void startSensorTask(TaskFunction_t sensorTask, const char * const sensorTaskName) {
    xTaskCreatePinnedToCore(sensorTask, sensorTaskName, configMINIMAL_STACK_SIZE * 8, NULL, 5, NULL, APP_CPU_NUM);
}


