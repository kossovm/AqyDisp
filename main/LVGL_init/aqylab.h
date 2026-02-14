#include <stdio.h>


#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_timer.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_panel_ops.h"

#include "driver/gpio.h"
#include <driver/ledc.h>
#include <driver/spi_master.h>

#include "esp_err.h"
#include "esp_log.h"
#include "esp_check.h"
#include "lvgl.h"
#include "lv_demos.h"
#include "esp_lvgl_port.h"
#include "esp_lcd_ili9488.h"

#include "esp_lcd_touch.h"

#include "driver/i2c_master.h"
#include "esp_lcd_touch_gt911.h"




////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////// Please update the following configuration according to your LCD spec //////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#define EXAMPLE_LCD_PIXEL_CLOCK_HZ     (15 * 1000 * 1000)
#define EXAMPLE_LCD_I80_BUS_WIDTH 16


#define EXAMPLE_LCD_BK_LIGHT_ON_LEVEL  1
#define EXAMPLE_LCD_BK_LIGHT_OFF_LEVEL !EXAMPLE_LCD_BK_LIGHT_ON_LEVEL
#define EXAMPLE_PIN_NUM_DATA0          1
#define EXAMPLE_PIN_NUM_DATA1          18
#define EXAMPLE_PIN_NUM_DATA2          2
#define EXAMPLE_PIN_NUM_DATA3          8
#define EXAMPLE_PIN_NUM_DATA4          38
#define EXAMPLE_PIN_NUM_DATA5          7
#define EXAMPLE_PIN_NUM_DATA6          16
#define EXAMPLE_PIN_NUM_DATA7          15
#if EXAMPLE_LCD_I80_BUS_WIDTH > 8
#define EXAMPLE_PIN_NUM_DATA8          17
#define EXAMPLE_PIN_NUM_DATA9          9
#define EXAMPLE_PIN_NUM_DATA10         48
#define EXAMPLE_PIN_NUM_DATA11         10
#define EXAMPLE_PIN_NUM_DATA12         13
#define EXAMPLE_PIN_NUM_DATA13         11
#define EXAMPLE_PIN_NUM_DATA14         14
#define EXAMPLE_PIN_NUM_DATA15         12
#endif
#define EXAMPLE_PIN_NUM_WR           21
#define EXAMPLE_PIN_NUM_CS             43
#define EXAMPLE_PIN_NUM_RS             44
#define EXAMPLE_PIN_NUM_RST            47
#define EXAMPLE_PIN_NUM_BK_LIGHT       4

// The pixel number in horizontal and vertical
#define EXAMPLE_LCD_H_RES              320
#define EXAMPLE_LCD_V_RES              480
// Bit number used to represent command and parameter
#define EXAMPLE_LCD_CMD_BITS           8
#define EXAMPLE_LCD_PARAM_BITS         8





#define EXAMPLE_TOUCH_RST       40         // Maybe shared with LCD's reset????? Or -1 if tied high
#define EXAMPLE_TOUCH_INT        39         // or -1 for polling
#define EXAMPLE_TOUCH_CLK_HZ 400000


#define BUFFER_SIZE (EXAMPLE_LCD_H_RES * EXAMPLE_LCD_V_RES) / 5


void aqylab_init(void);
esp_err_t i2c_MAIN_init(void);