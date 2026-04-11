#include "GUI/ui.h"
#include "LVGL_init/aqylab.h"
#include "OTA/ota.h"
#include "WIFI/direct_wifi.h"
#include "Sensors/sensors.h"
#include "JSON/fetchData.h"
#include "esp_wifi.h"
#include "esp_netif.h"
#include "config_app.h"
#include "WIFI/captive_portal.h"

static const char *TAG = "MAIN";
static OrderData currentData = { .order_id = -1 };

typedef enum {
    VIEW_WAITING,
    VIEW_READY,
    VIEW_THANK_YOU,
    VIEW_ERROR,
    VIEW_PORTAL
} AppViewStatus;

static AppViewStatus app_state = VIEW_WAITING;

static void ui_state_update_timer_cb(lv_timer_t * timer) {
    static uint32_t ms_counter = 0;
    ms_counter += 100; // Timer runs every 100ms (4 FPS - Ultra safe for 24/7)
    
    // Pulse animation logic (4000ms cycle)
    uint32_t phase = ms_counter % 4000;
    uint32_t ratio = phase < 2000 ? (phase * 255 / 2000) : ((4000 - phase) * 255 / 2000);
    
    // Language logic (swap every 4 seconds)
    bool is_kazakh = (ms_counter / 4000) % 2 != 0;
    
    lv_color_t target_color = lv_color_black();
    char main_text[256] = "";

    switch (app_state) {
        case VIEW_WAITING:
            target_color = lv_color_hex(0x2b7fff);
            snprintf(main_text, sizeof(main_text), is_kazakh ? "Тапсырысыңызды\nкүтеміз" : "Ожидаем\nзаказ");
            lv_obj_set_y(ui_readyNameLabel, 0); // Center
            lv_label_set_text(ui_readyNumberLabel, "");
            break;
            
        case VIEW_READY:
            target_color = lv_color_hex(0x00c950);
            snprintf(main_text, sizeof(main_text), "%s", currentData.customer_name);
            lv_obj_set_y(ui_readyNameLabel, 70); // Имя снизу
            break;
            
        case VIEW_THANK_YOU:
            target_color = lv_color_hex(0xfe9a00);
            snprintf(main_text, sizeof(main_text), "%s,\n%s!", currentData.customer_name, is_kazakh ? "Тапсырыс үшін\nрахмет" : "Спасибо\nза заказ");
            lv_obj_set_y(ui_readyNameLabel, 0); // Center
            lv_label_set_text(ui_readyNumberLabel, "");
            break;
            
        case VIEW_ERROR:
            target_color = lv_color_black();
            lv_obj_set_y(ui_readyNameLabel, 0); // Center
            lv_label_set_text(ui_readyNumberLabel, "");
            break;
            
        case VIEW_PORTAL:
            target_color = lv_color_hex(0x2b7fff);
            ratio = 255; // Static
            break;
    }
    
    // Theme overrrides
    lv_color_t text_color = lv_color_white();
    if (app_state != VIEW_PORTAL) {
        if (g_app_config.theme_id == 2) {
            target_color = lv_color_black();
            ratio = 255;
        } else if (g_app_config.theme_id == 3) {
            target_color = lv_color_white();
            text_color = lv_color_black();
            ratio = 255;
        }
    }
    
    // Output mixed color
    lv_color_t mixed = lv_color_mix(target_color, lv_color_black(), ratio);
    lv_obj_set_style_bg_color(ui_ServiceScreen, mixed, LV_PART_MAIN);
    
    // Set Text Color
    lv_obj_set_style_text_color(ui_readyNameLabel, text_color, LV_PART_MAIN);
    lv_obj_set_style_text_color(ui_readyNumberLabel, text_color, LV_PART_MAIN);
    
    // Output text if not error
    if (app_state != VIEW_ERROR && app_state != VIEW_PORTAL) {
        lv_label_set_text(ui_readyNameLabel, main_text);
    }
}

// Engineering Menu Actions
static void eng_reset_wifi_cb(lv_event_t * e) {
    config_app_reset();
    config_app_save();
    esp_restart();
}

static void eng_reboot_cb(lv_event_t * e) {
    esp_restart();
}

static void eng_menu_cb(lv_event_t * e) {
    lv_indev_t * indev = lv_indev_active();
    if(indev == NULL) return;
    
    lv_point_t p;
    lv_indev_get_point(indev, &p);

    // If Top Right corner (approx 100x100)
    if (p.x > 300 && p.y < 100) {
        ESP_LOGI(TAG, "Entering Engineering Menu!");
        
        lv_obj_t * overlay = lv_obj_create(lv_screen_active());
        lv_obj_set_size(overlay, lv_pct(80), lv_pct(80));
        lv_obj_align(overlay, LV_ALIGN_CENTER, 0, 0);
        lv_obj_set_style_bg_color(overlay, lv_color_white(), 0);
        
        lv_obj_t * title = lv_label_create(overlay);
        lv_label_set_text(title, "Engineering Menu");
        lv_obj_set_style_text_color(title, lv_color_black(), 0);
        lv_obj_set_style_text_font(title, &lv_font_montserrat_14, 0); // basic font
        lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 10);
        
        // WiFi Reset Btn
        lv_obj_t * btn1 = lv_button_create(overlay);
        lv_obj_set_size(btn1, 250, 60);
        lv_obj_align(btn1, LV_ALIGN_CENTER, 0, -35);
        lv_obj_t * l1 = lv_label_create(btn1);
        lv_label_set_text(l1, "Erase Memory (WiFi Reset)");
        lv_obj_set_style_text_color(l1, lv_color_black(), 0);
        lv_obj_center(l1);
        lv_obj_add_event_cb(btn1, eng_reset_wifi_cb, LV_EVENT_CLICKED, NULL);

        // Reboot Btn
        lv_obj_t * btn2 = lv_button_create(overlay);
        lv_obj_set_size(btn2, 250, 60);
        lv_obj_align(btn2, LV_ALIGN_CENTER, 0, 35);
        lv_obj_t * l2 = lv_label_create(btn2);
        lv_label_set_text(l2, "Reboot System");
        lv_obj_set_style_text_color(l2, lv_color_black(), 0);
        lv_obj_center(l2);
        lv_obj_add_event_cb(btn2, eng_reboot_cb, LV_EVENT_CLICKED, NULL);
    }
}


void api_update_task(void *pvParameters) {
    OrderData newData;
    char buffer_number[64];

    memset(&newData, 0, sizeof(newData)); 
    int network_fail_count = 0;
    int dot_count = 0;

    while (1) {
        ESP_LOGI(TAG, "Checking for API updates...");

        // 1. Fetch the data (Blocking HTTP request)
        // We do NOT lock LVGL here because HTTP is slow and we want the UI to remain responsive (animations, etc)
        esp_err_t result = fetch_order_data(&newData);

        if (result == ESP_OK) {
            network_fail_count = 0;
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

                if (newData.order_id <= 0 && strlen(newData.customer_name) == 0 && strlen(newData.order_number) == 0) {
                    // Сценарий: очередь пуста
                    if (currentData.order_id > 0) {
                        app_state = VIEW_THANK_YOU;
                        lvgl_port_unlock();
                        vTaskDelay(pdMS_TO_TICKS(16000)); // Ждем 16 секунд (4 цикла переливания)
                        lvgl_port_lock(0);
                    }
                    app_state = VIEW_WAITING;
                } else {
                    app_state = VIEW_READY;
                    // Format number immediately since timer handles text and position
                    snprintf(buffer_number, sizeof(buffer_number), "%s", newData.order_number);
                    lv_label_set_text(ui_readyNumberLabel, buffer_number);
                    lv_obj_set_y(ui_readyNumberLabel, -30); // Номер сверху (оригинальное положение)
                }


                lvgl_port_unlock();

                // 4. Update our local cache
                currentData = newData;
            } else {
                ESP_LOGD(TAG, "Data is identical. No update needed.");
            }
        } else {
            ESP_LOGE(TAG, "Failed to fetch data from API");
            network_fail_count++;
            
            // Анимация точек: . .. ...
            dot_count = (dot_count % 3) + 1;
            char loader_text[64];
            snprintf(loader_text, sizeof(loader_text), "Желіге қосылуда");
            for(int i = 0; i < dot_count; i++) {
                strcat(loader_text, ".");
            }

            lvgl_port_lock(0);
            lv_obj_set_style_text_font(ui_readyNameLabel, &ui_font_KazakhFontNovo36, LV_PART_MAIN);
            lv_obj_set_style_text_font(ui_readyNumberLabel, &ui_font_KazakhFontNovoNumbers140, LV_PART_MAIN);
            
            app_state = VIEW_ERROR;
            
            lv_label_set_text(ui_readyNameLabel, loader_text);
            lv_label_set_text(ui_readyNumberLabel, "");
            lvgl_port_unlock();

            // Принудительная попытка переподключения Wi-Fi на всякий случай
            esp_wifi_connect();

            memset(&currentData, 0, sizeof(currentData));
            currentData.order_id = -2; // State indicating retry/error
        }

        // 6. Wait for 2 seconds (2000ms) before next check
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}






void app_main(void)
{

    //scd41_test(NULL);

    //xTaskCreatePinnedToCore(scd41_test, "testing", configMINIMAL_STACK_SIZE * 8, NULL, 5, NULL, APP_CPU_NUM);

    // i2c_MAIN_init();

    aqylab_init();

    // 1. Initialise Global Network Stack (required for both STA and AP)
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    lvgl_port_lock(0);

    ui_init();

    // Экран ожидания при старте
    lv_obj_set_style_text_font(ui_readyNameLabel, &ui_font_KazakhFontNovo36, LV_PART_MAIN);
    lv_obj_set_style_text_font(ui_readyNumberLabel, &ui_font_KazakhFontNovoNumbers140, LV_PART_MAIN);
    
    // Создаем обработчик долгого нажатия для скрытого меню
    lv_obj_add_event_cb(ui_ServiceScreen, eng_menu_cb, LV_EVENT_LONG_PRESSED, NULL);

    // Центрируем многострочный текст и устанавливаем выравнивание по умолчанию
    lv_obj_set_align(ui_readyNameLabel, LV_ALIGN_CENTER);
    lv_obj_set_style_text_align(ui_readyNameLabel, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_align(ui_readyNumberLabel, LV_ALIGN_CENTER);

    lv_timer_create(ui_state_update_timer_cb, 100, NULL);
    lvgl_port_unlock();

    bool has_config = config_app_load();

    if (!has_config || strlen(g_app_config.ssid) == 0) {
        // Запуск Captive Portal
        app_state = VIEW_PORTAL;
        start_captive_portal();
        
        lvgl_port_lock(0);
        lv_obj_set_y(ui_readyNameLabel, 0);
        lv_label_set_text(ui_readyNameLabel, "Подключись к Wi-Fi:\nCoffee_Setup\nи перейди на 192.168.4.1");
        lv_label_set_text(ui_readyNumberLabel, ""); // Remove #---
        lvgl_port_unlock();

        while(1) { vTaskDelay(1000); }
    } else {
        // Обычный запуск
        wifi_connect_direct(g_app_config.ssid, g_app_config.password);
        xTaskCreate(api_update_task, "API_Task", 8192, NULL, 5, NULL);
    }
}