#include "GUI/ui.h"
#include "LVGL_init/aqylab.h"
#include "OTA/ota.h"
#include "WIFI/wifi.h"
#include "Sensors/sensors.h"

void app_main(void)
{

    //scd41_test(NULL);

    //xTaskCreatePinnedToCore(scd41_test, "testing", configMINIMAL_STACK_SIZE * 8, NULL, 5, NULL, APP_CPU_NUM);

    // i2c_MAIN_init();

    aqylab_init();

    lvgl_port_lock(0);

    ui_init();

    // wifi_ui_init();

    // lv_demo_stress();

    lvgl_port_unlock();


}
