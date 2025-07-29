#include "sensors.h"

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


esp_err_t i2c_master_dev_init(i2c_master_bus_handle_t *i2c_bus_handle, i2c_master_dev_handle_t *i2c_device_handle, uint16_t i2c_device_address, uint32_t i2c_device_frequency) {
    i2c_device_config_t i2c_device_config = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = i2c_device_address,
        .scl_speed_hz = i2c_device_frequency,
    };

    ESP_ERROR_CHECK(i2c_master_bus_add_device(*i2c_bus_handle, &i2c_device_config, i2c_device_handle));

    return ESP_OK;
}