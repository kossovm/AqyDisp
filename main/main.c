#include "GUI/ui.h"
#include "LVGL_init/aqylab.h"
#include "OTA/ota.h"

void app_main(void)
{

    ota_custom_init();

    aqylab_init();

    lvgl_port_lock(0);

    ui_init();

    lvgl_port_unlock();


}
