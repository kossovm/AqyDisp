/*
 * SPDX-FileCopyrightText: 2021 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#include "aqylab.h"
#include "Sensors/sensors.h"
#include "Sensors/i2c_shared.h"

static esp_lcd_touch_handle_t touch_handle = NULL;
static lv_indev_t *lvgl_touch_indev = NULL;


static esp_lcd_panel_io_handle_t io_handle = NULL;
static esp_lcd_panel_handle_t panel_handle = NULL;

static lv_disp_t * disp_handle;

extern void example_lvgl_demo_ui(lv_disp_t *scr);

static const char *TAG = "AqyLab_Display";


esp_err_t i2c_MAIN_init(void)
{

    //// BUS INIT + DUMMY DEV for i2c bus initialization  ////////////////

    esp_idf_lib_i2cdev_init();

    initiateDummyDeviceForBUUUUS();

    ///////////////////////////////////////////////////////////////////

    esp_err_t err = ESP_OK;

    return err;
}

esp_err_t app_touch_init(void)
{   
    //i2c_init_once();

    //i2c_master_bus_init(&i2c_bus_handle);


    i2c_bus_handle = i2cdev_get_bus_handle(1);

    /* Initialize touch */
    esp_lcd_panel_io_i2c_config_t tp_io_config = ESP_LCD_TOUCH_IO_I2C_GT911_CONFIG();

    tp_io_config.scl_speed_hz = EXAMPLE_TOUCH_CLK_HZ;

    esp_lcd_touch_io_gt911_config_t tp_gt911_config = {
        .dev_addr = tp_io_config.dev_addr,
    };

    const esp_lcd_touch_config_t tp_cfg = {
        .x_max = EXAMPLE_LCD_H_RES,
        .y_max = EXAMPLE_LCD_V_RES,
        .rst_gpio_num = EXAMPLE_TOUCH_RST,
        .int_gpio_num = EXAMPLE_TOUCH_INT,
        .levels = {
            .reset = 0,
            .interrupt = 0,
        },
        .flags = {
            .swap_xy = 0,
            .mirror_x = 0,
            .mirror_y = 0,
        },
        .driver_data = &tp_gt911_config,
    };


    esp_lcd_panel_io_handle_t tp_io_handle = NULL;


    ESP_RETURN_ON_ERROR(esp_lcd_new_panel_io_i2c(i2c_bus_handle, &tp_io_config, &tp_io_handle), TAG, "Couldn't create a new i2c panel for touch");


    esp_err_t err = esp_lcd_touch_new_i2c_gt911(tp_io_handle, &tp_cfg, &touch_handle);

    /* Add touch input (for selected screen) */
    const lvgl_port_touch_cfg_t touch_cfg = {
        .disp = disp_handle,
        .handle = touch_handle,
    };
    lvgl_touch_indev = lvgl_port_add_touch(&touch_cfg);

    return err;
}


esp_err_t lcd_core_init(void) {

    ESP_LOGI(TAG, "Turning OFF backlight");

    
    gpio_config_t bk_gpio_config = {
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask = 1ULL << EXAMPLE_PIN_NUM_BK_LIGHT
    };
    ESP_ERROR_CHECK(gpio_config(&bk_gpio_config));
    gpio_set_level(EXAMPLE_PIN_NUM_BK_LIGHT, EXAMPLE_LCD_BK_LIGHT_OFF_LEVEL);

    // // New IO Bus

    ESP_LOGI(TAG, "Initialize Intel 8080 bus");
    esp_lcd_i80_bus_handle_t i80_bus = NULL;
    esp_lcd_i80_bus_config_t bus_config = {
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
        .clk_src = LCD_CLK_SRC_DEFAULT,
#endif
        .dc_gpio_num = EXAMPLE_PIN_NUM_RS,
        .wr_gpio_num = EXAMPLE_PIN_NUM_WR,
        .data_gpio_nums = {
            EXAMPLE_PIN_NUM_DATA0,
            EXAMPLE_PIN_NUM_DATA1,
            EXAMPLE_PIN_NUM_DATA2,
            EXAMPLE_PIN_NUM_DATA3,
            EXAMPLE_PIN_NUM_DATA4,
            EXAMPLE_PIN_NUM_DATA5,
            EXAMPLE_PIN_NUM_DATA6,
            EXAMPLE_PIN_NUM_DATA7,
#if EXAMPLE_LCD_I80_BUS_WIDTH > 8
            EXAMPLE_PIN_NUM_DATA8,
            EXAMPLE_PIN_NUM_DATA9,
            EXAMPLE_PIN_NUM_DATA10,
            EXAMPLE_PIN_NUM_DATA11,
            EXAMPLE_PIN_NUM_DATA12,
            EXAMPLE_PIN_NUM_DATA13,
            EXAMPLE_PIN_NUM_DATA14,
            EXAMPLE_PIN_NUM_DATA15,
#endif
        },
        .bus_width = EXAMPLE_LCD_I80_BUS_WIDTH,
        .max_transfer_bytes = BUFFER_SIZE * sizeof(uint16_t)
    };

    ESP_RETURN_ON_ERROR(esp_lcd_new_i80_bus(&bus_config, &i80_bus), TAG, "Couldn't create i80 bus");

    // New IO panel
    
    esp_lcd_panel_io_i80_config_t io_config = {
        .cs_gpio_num = EXAMPLE_PIN_NUM_CS,
        .pclk_hz = EXAMPLE_LCD_PIXEL_CLOCK_HZ,
        .trans_queue_depth = 10,
        .dc_levels = {
            .dc_idle_level = 0,
            .dc_cmd_level = 0,
            .dc_dummy_level = 0,
            .dc_data_level = 1,
        },
        .lcd_cmd_bits = EXAMPLE_LCD_CMD_BITS,
        .lcd_param_bits = EXAMPLE_LCD_PARAM_BITS,
    };
    ESP_RETURN_ON_ERROR(esp_lcd_new_panel_io_i80(i80_bus, &io_config, &io_handle), TAG, "Couldn't create io panel");


    // Install LCD Driver

    ESP_LOGI(TAG, "Install LCD driver of ili9488");
    
    esp_lcd_panel_dev_config_t panel_config = {
        .reset_gpio_num = EXAMPLE_PIN_NUM_RST,
        .color_space = ESP_LCD_COLOR_SPACE_BGR,
        .bits_per_pixel = 16,
    };
    ESP_RETURN_ON_ERROR(esp_lcd_new_panel_ili9488(io_handle, &panel_config, 0, &panel_handle), TAG, "Couldn't initialiaze ili9488 panel");


    esp_lcd_panel_reset(panel_handle);
    esp_lcd_panel_init(panel_handle);


    ESP_LOGI(TAG, "Turn on LCD backlight");
    gpio_set_level(EXAMPLE_PIN_NUM_BK_LIGHT, EXAMPLE_LCD_BK_LIGHT_ON_LEVEL);

    return ESP_OK;
}

esp_err_t lcd_port_init(void) {

    const lvgl_port_cfg_t lvgl_cfg = ESP_LVGL_PORT_INIT_CONFIG();
    esp_err_t err = lvgl_port_init(&lvgl_cfg);

    const lvgl_port_display_cfg_t disp_cfg = {
        .io_handle = io_handle,
        .panel_handle = panel_handle,
        .buffer_size = BUFFER_SIZE,
        .double_buffer = true,
        .hres = EXAMPLE_LCD_H_RES,
        .vres = EXAMPLE_LCD_V_RES,
        .color_format = LV_COLOR_FORMAT_RGB565,
        .rotation = {
            .swap_xy = false,
            .mirror_x = false,
            .mirror_y = false,
        },
        .flags = {
            .buff_dma = true,
            .swap_bytes = false,
        }
    };
    disp_handle = lvgl_port_add_disp(&disp_cfg);

    return err;
}

void aqylab_init(void)
{
    // Backbone

    ESP_ERROR_CHECK(i2c_MAIN_init());
    
    ESP_ERROR_CHECK(lcd_core_init());

    ESP_ERROR_CHECK(lcd_port_init());

    ESP_ERROR_CHECK(app_touch_init());


    lvgl_port_lock(0);

    lv_disp_set_rotation(disp_handle, LV_DISP_ROTATION_270);

    lvgl_port_unlock();


    // MAIN APP

    ESP_LOGI(TAG, "SUCCESS!");

}










