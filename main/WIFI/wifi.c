#include "esp_wifi.h"
#include "esp_netif.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs.h"
#include "nvs_flash.h"

#include "GUI/ui.h"           // SquareLine generated UI header (for ui_wifiPanel, etc.)
#include <string.h>

static const char *TAG = "WiFiUI";

static char selected_ssid[33] = "";       // Buffer to store the selected SSID

static lv_obj_t *wifi_list = NULL;       // LVGL list object to display networks
static bool scan_in_progress = false;    // Flag to avoid overlapping scans

/** Structure to pass scan results to LVGL async callback **/
typedef struct {
    uint16_t count;
    wifi_ap_record_t *records;
} scan_result_t;



/** Callback for when a Wi-Fi list item (network SSID) is clicked */
void wifiList_item_event_cb(lv_event_t *e) {
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
    lv_obj_t *btn = lv_event_get_target(e);
    const char *ssid = lv_list_get_btn_text(wifi_list, btn);  // get the SSID text from the clicked button
    if (ssid != NULL) {
        // Store the selected SSID for use when connecting
        strncpy(selected_ssid, ssid, sizeof(selected_ssid) - 1);
        selected_ssid[sizeof(selected_ssid) - 1] = '\0';  // ensure null termination
        ESP_LOGI(TAG, "Selected SSID: %s", selected_ssid);
        // (Optional: provide visual feedback in UI that this network was selected)
    }

    if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
        CustomFadeIn_Animation(ui_wifiEnterPanel, 0);
        _ui_flag_modify(ui_wifiMenu, LV_OBJ_FLAG_HIDDEN, _UI_MODIFY_FLAG_ADD);
        _ui_flag_modify(ui_wifiEnterPanel, LV_OBJ_FLAG_HIDDEN, _UI_MODIFY_FLAG_REMOVE);
        lv_label_set_text(ui_selectedWiFiLabel, selected_ssid);
    }
}


/** LVGL async callback to update the Wi-Fi list UI (runs in LVGL task context) */
static void update_wifi_list_cb(void *user_data) {
    scan_result_t *res = (scan_result_t *)user_data;
    uint16_t count = 0;
    wifi_ap_record_t *ap_list = NULL;
    if (res != NULL) {
        count = res->count;
        ap_list = res->records;
    }

    // Ensure the list object exists
    if (!wifi_list) {
        ESP_LOGW(TAG, "Wi-Fi list UI object not found!");
        // Free memory if allocated
        if (res) {
            free(res->records);
            free(res);
        }
        return;
    }

    // Clear any previous items in the list
    lv_obj_clean(wifi_list);

    if (count == 0) {
        // No networks found: show a placeholder message
        lv_list_add_text(wifi_list, "No networks found");
    } else {
        // Populate the list with new scan results
        for (int i = 0; i < count; ++i) {
            if (ap_list[i].ssid[0] == '\0') {
                continue; // skip hidden SSIDs (empty string)
            }
            const char *ssid = (const char *)ap_list[i].ssid;
            // Add a new list item with Wi-Fi icon and SSID text
            lv_obj_t *btn = lv_list_add_btn(wifi_list, LV_SYMBOL_WIFI, ssid);
            lv_obj_add_event_cb(btn, wifiList_item_event_cb, LV_EVENT_CLICKED, NULL);
            // If the network is secured (not open), append a lock symbol to its label
            if (ap_list[i].authmode != WIFI_AUTH_OPEN) {
                lv_obj_t *label = lv_obj_get_child(btn, 0);  // get the label child of the button
                lv_label_set_text_fmt(label, "%s  " LV_SYMBOL_CLOSE, ssid);
            }
        }
    }

    // Free the scan results memory
    if (res) {
        free(res->records);
        free(res);
    }
    scan_in_progress = false;  // scanning is complete
}

/** Event handler for Wi-Fi and IP events (from ESP-IDF event loop) */
static void wifi_event_handler(void *arg, esp_event_base_t event_base, 
                               int32_t event_id, void *event_data) {
    if (event_base == WIFI_EVENT) {
        switch (event_id) {
            case WIFI_EVENT_STA_START:
                ESP_LOGI(TAG, "Wi-Fi started in STA mode");
                break;
            case WIFI_EVENT_STA_CONNECTED:
                ESP_LOGI(TAG, "Wi-Fi connected to AP");
                // Update UI or trigger next steps if needed
                break;
            case WIFI_EVENT_STA_DISCONNECTED: {
                wifi_event_sta_disconnected_t *disconn = event_data;
                ESP_LOGW(TAG, "Wi-Fi disconnected (reason=%d)", disconn->reason);
                // Could trigger auto-reconnect or update UI here if desired
                break;
            }
            case WIFI_EVENT_SCAN_DONE: {
                // Scan completed (results ready)
                uint16_t ap_count = 0;
                esp_wifi_scan_get_ap_num(&ap_count);
                ESP_LOGI(TAG, "Wi-Fi scan done, %d APs found", ap_count);

                // Allocate memory for AP records and fetch them
                wifi_ap_record_t *ap_list = NULL;
                if (ap_count > 0) {
                    ap_list = malloc(ap_count * sizeof(wifi_ap_record_t));
                }
                scan_result_t *res = malloc(sizeof(scan_result_t));
                if (res == NULL) {
                    ESP_LOGE(TAG, "Failed to allocate memory for scan results");
                    if (ap_list) free(ap_list);
                    scan_in_progress = false;
                    break;
                }
                res->count = ap_count;
                res->records = ap_list;
                if (ap_list && ap_count > 0) {
                    esp_err_t err = esp_wifi_scan_get_ap_records(&ap_count, ap_list);
                    if (err != ESP_OK) {
                        ESP_LOGE(TAG, "esp_wifi_scan_get_ap_records failed: %d", err);
                    }
                    // Note: esp_wifi_scan_get_ap_records frees the internal scan list.
                    // We now have a local copy in ap_list (regardless of success or fail).
                }

                // Use LVGL async call to update UI in the GUI thread safely
                lv_async_call(update_wifi_list_cb, res);
                break;
            }
            default:
                break;
        }
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *ip_event = event_data;
        // ESP_LOGI(TAG, "Got IP: %s", esp_ip4addr_ntoa(&ip_event->ip_info.ip));
        // Could update UI with IP address if needed
    }
}

/** Initialize NVS (for Wi-Fi storage) */
static void init_nvs(void) {
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        // NVS partition was full or had an incompatible version, erase and re-init
        ESP_ERROR_CHECK(nvs_flash_erase());
        ESP_ERROR_CHECK(nvs_flash_init());
    }
}

/** Create the LVGL list object inside the designated panel (ui_wifiPanel) */
void create_wifi_list(void) {
    lv_obj_t *parent = ui_wifiListPanel;                      // parent panel for the list
    wifi_list = lv_list_create(parent);                   // create a new list
    lv_obj_set_size(wifi_list, lv_pct(100), lv_pct(100));  // make list fill 100% width, 70% height of panel
    lv_obj_align(wifi_list, LV_ALIGN_TOP_MID, 0, 0);     // position it 0 px from the panel's top
    // Optional: style adjustments for appearance
    lv_obj_set_style_pad_row(wifi_list, 4, 0);            // set row padding
    lv_obj_set_scroll_dir(wifi_list, LV_DIR_VER);         // enable vertical scrolling only
    lv_obj_set_style_bg_color(wifi_list, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_border_width(wifi_list, 0, 0);
    // Enable scrolling via touch (so the list can be dragged)
    lv_obj_add_flag(wifi_list, LV_OBJ_FLAG_SCROLL_ELASTIC);
}



/** Event callback for the "Scan" button - starts a Wi-Fi scan */
void scanWiFi_btn_event_cb(lv_event_t *e) {

    lv_event_code_t event_code = lv_event_get_code(e);
    lv_obj_t * target = lv_event_get_target(e);


    if(event_code == LV_EVENT_VALUE_CHANGED &&  lv_obj_has_state(target, LV_STATE_CHECKED)) {
        if (scan_in_progress) {
            ESP_LOGW(TAG, "Scan already in progress, please wait...");
            return;
        }

        ESP_LOGI(TAG, "Starting Wi-Fi scan...");

        // Configure scan parameters (scan all channels, no filters)
        wifi_scan_config_t scan_config = {
            .ssid = NULL,
            .bssid = NULL,
            .channel = 0,        // 0 = scan all channels
            .show_hidden = false // do not show hidden SSIDs
            // .scan_type and .scan_time use default (active scan, 120ms per channel)
        };


        scan_in_progress = true;
        _ui_flag_modify(ui_wifiScanningImage, LV_OBJ_FLAG_HIDDEN, _UI_MODIFY_FLAG_REMOVE);
        _ui_flag_modify(ui_wifiScanningLabel, LV_OBJ_FLAG_HIDDEN, _UI_MODIFY_FLAG_REMOVE);

        esp_err_t err = esp_wifi_scan_start(&scan_config, false /* non-blocking */);



        if (err != ESP_OK) {
            scan_in_progress = false;
            ESP_LOGE(TAG, "esp_wifi_scan_start failed: %d", err);
            // Optionally, update UI to show an error message
        }

        if (err == ESP_OK) {
            
            lv_label_set_text(ui_wifiScanningLabel, "Done!");
            vTaskDelay(pdMS_TO_TICKS(500));
            lv_label_set_text(ui_wifiScanningLabel, "Scanning...");
            _ui_flag_modify(ui_wifiScanningImage, LV_OBJ_FLAG_HIDDEN, _UI_MODIFY_FLAG_ADD);
            _ui_flag_modify(ui_wifiScanningLabel, LV_OBJ_FLAG_HIDDEN, _UI_MODIFY_FLAG_ADD);

            scan_in_progress = false;
            ESP_LOGE(TAG, "Success!");
        }    
    }

}

/** Event callback for the "Connect" button - attempts to connect to the selected network */
void connectWiFi_btn_event_cb(lv_event_t *e) {
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
    if (selected_ssid[0] == '\0') {
        ESP_LOGW(TAG, "No Wi-Fi network selected for connecting.");
        // Optionally, show a UI message like "Please select a network"
        return;
    }
    // Get the password from the text input field on the UI
    const char *password = lv_textarea_get_text(ui_wifiPasswordTextInput);
    ESP_LOGI(TAG, "Connecting to SSID: %s...", selected_ssid);
    // Set up Wi-Fi config with the selected SSID and entered password
    wifi_config_t wifi_config = { 0 };
    strncpy((char*)wifi_config.sta.ssid, selected_ssid, sizeof(wifi_config.sta.ssid) - 1);
    strncpy((char*)wifi_config.sta.password, password, sizeof(wifi_config.sta.password) - 1);
    wifi_config.sta.threshold.authmode = WIFI_AUTH_OPEN;   // allow all auth modes (if password is empty, still attempt)
    wifi_config.sta.pmf_cfg.capable = true;
    wifi_config.sta.pmf_cfg.required = false;
    // Apply the STA configuration
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    // Initiate connection
    esp_err_t err = esp_wifi_connect();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_wifi_connect failed: %d", err);
        // You might want to handle connection failures (e.g., wrong password) in WIFI_EVENT_STA_DISCONNECTED events
    }
    // Connection result (success/fail) will be reported via events (handled in wifi_event_handler)
}



void passwordBringKeyboard_cb(lv_event_t *e) {
    if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
        _ui_flag_modify(ui_wifiKeyboard, LV_OBJ_FLAG_HIDDEN, _UI_MODIFY_FLAG_REMOVE);
    }
}








/** Initialize Wi-Fi (NVS, netif, event loop, Wi-Fi driver) and UI components */
void wifi_ui_init(void) {
    init_nvs();              // Initialize NVS for Wi-Fi storage
    create_wifi_list();      // Create the LVGL list object in the UI panel

    // Initialize TCP/IP stack and default event loop
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();  // create default Wi-Fi STA netif

    // Initialize the Wi-Fi driver with default configuration
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    // Configure Wi-Fi in Station mode
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    // Disable auto-connect to avoid automatic connection on startup (we'll connect manually)

    // Start Wi-Fi
    ESP_ERROR_CHECK(esp_wifi_start());

    // Register event handlers for Wi-Fi and IP events (on the default loop)
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, 
                    wifi_event_handler, NULL, NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, ESP_EVENT_ANY_ID, 
                    wifi_event_handler, NULL, NULL));

    // Attach LVGL UI button events to our callbacks
    lv_obj_add_event_cb(ui_wifiSwitch,    scanWiFi_btn_event_cb,    LV_EVENT_VALUE_CHANGED, NULL);
    lv_obj_add_event_cb(ui_connectWifiButton, connectWiFi_btn_event_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_add_event_cb(ui_wifiPasswordTextInput, passwordBringKeyboard_cb, LV_EVENT_CLICKED, NULL);
}
