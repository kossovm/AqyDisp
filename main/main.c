#include "GUI/ui.h"
#include "LVGL_init/aqylab.h"
#include "OTA/ota.h"
#include "WIFI/direct_wifi.h"
#include "Sensors/sensors.h"
#include "JSON/fetchData.h"






static OrderData currentData = { .order_id = -1 };

void api_update_task(void *pvParameters) {
    OrderData newData;
    char buffer_name[160];
    char buffer_number[64];

    memset(&newData, 0, sizeof(newData)); 

    while (1) {
        ESP_LOGI(TAG, "Checking for API updates...");

        // 1. Fetch the data (Blocking HTTP request)
        // We do NOT lock LVGL here because HTTP is slow and we want the UI to remain responsive (animations, etc)
        esp_err_t result = fetch_order_data(&newData);

        if (result == ESP_OK) {
            // 2. Check if data has changed
            // We compare ID, Number, and Name to be sure
            bool isChanged = (newData.order_id != currentData.order_id) ||
                             (strcmp(newData.order_number, currentData.order_number) != 0) ||
                             (strcmp(newData.customer_name, currentData.customer_name) != 0);

            if (isChanged) {
                ESP_LOGI(TAG, "New data found! Updating Screen...");

                // 3. Lock LVGL only for the short time we need to write to the screen
                lvgl_port_lock(0);

                // Setup Styles (only strictly needed once, but safe to do here)
                lv_obj_set_style_text_font(ui_readyNameLabel, &ui_font_KazakhFontNovo36, LV_PART_MAIN);
                lv_obj_set_style_text_font(ui_readyNumberLabel, &ui_font_KazakhFontNovoNumbers140, LV_PART_MAIN);

                // Format strings
                snprintf(buffer_name, sizeof(buffer_name), "%s", newData.customer_name);
                snprintf(buffer_number, sizeof(buffer_number), "%s", newData.order_number);

                // Set Text
                lv_label_set_text(ui_readyNameLabel, buffer_name);
                lv_label_set_text(ui_readyNumberLabel, buffer_number);

                lvgl_port_unlock();

                // 4. Update our local cache
                currentData = newData;
            } else {
                ESP_LOGD(TAG, "Data is identical. No update needed.");
            }
        } else {
            ESP_LOGE(TAG, "Failed to fetch data from API");
            // Optional: Update UI to show connection error?
        }

        // 5. Wait for 5 seconds (5000ms) before next check
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}






void app_main(void)
{

    //scd41_test(NULL);

    //xTaskCreatePinnedToCore(scd41_test, "testing", configMINIMAL_STACK_SIZE * 8, NULL, 5, NULL, APP_CPU_NUM);

    // i2c_MAIN_init();

    aqylab_init();

    wifi_connect_direct("Genei Ryodan", "baguette");

    lvgl_port_lock(0);

    ui_init();

    // lv_demo_stress();

    lvgl_port_unlock();

    xTaskCreate(api_update_task, "API_Task", 8192, NULL, 5, NULL);


}