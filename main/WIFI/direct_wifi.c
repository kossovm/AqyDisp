#include "esp_wifi.h"
#include "esp_netif.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs.h"
#include "nvs_flash.h"
#include <string.h>

// If you still want to update the UI status (e.g. show "Connected"), keep this.
// Otherwise, you can comment it out.
#include "GUI/ui.h" 

static const char *TAG = "WiFi_Auto";
static bool wifi_initialized = false;

/** Initialize NVS (Required for Wi-Fi) */
static void init_nvs(void) {
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ESP_ERROR_CHECK(nvs_flash_init());
    }
}

/** Event handler for Wi-Fi and IP events */
static void wifi_event_handler(void *arg, esp_event_base_t event_base, 
                               int32_t event_id, void *event_data) {
    if (event_base == WIFI_EVENT) {
        switch (event_id) {
            case WIFI_EVENT_STA_START:
                ESP_LOGI(TAG, "Wi-Fi started. Connecting...");
                esp_wifi_connect(); // Attempt connection immediately upon start
                break;
            case WIFI_EVENT_STA_CONNECTED:
                ESP_LOGI(TAG, "Connected to AP. Waiting for IP...");
                // Optional: Update UI to show connecting state
                // lv_label_set_text(ui_statusLabel, "Obtaining IP...");
                break;
            case WIFI_EVENT_STA_DISCONNECTED: {
                wifi_event_sta_disconnected_t *disconn = event_data;
                ESP_LOGW(TAG, "Disconnected (Reason: %d). Retrying...", disconn->reason);
                
                // Optional: Update UI to show failure
                // lv_label_set_text(ui_statusLabel, "Disconnected");

                // Simple auto-reconnect logic
                esp_wifi_connect(); 
                break;
            }
            default:
                break;
        }
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *ip_event = event_data;
        ESP_LOGI(TAG, "Success! Got IP: " IPSTR, IP2STR(&ip_event->ip_info.ip));
        
        // Optional: Update UI with IP
        // lv_label_set_text_fmt(ui_selectedWiFiLabel, "IP: " IPSTR, IP2STR(&ip_event->ip_info.ip));
    }
}

/** 
 * MAIN FUNCTION: Call this with your SSID and Password.
 * It handles initialization automatically on the first run.
 */
void wifi_connect_direct(const char *ssid, const char *password) {
    if (ssid == NULL) {
        ESP_LOGE(TAG, "SSID cannot be NULL");
        return;
    }

    // 1. One-time Initialization of the stack
    if (!wifi_initialized) {
        init_nvs();
        ESP_ERROR_CHECK(esp_netif_init());
        ESP_ERROR_CHECK(esp_event_loop_create_default());
        esp_netif_create_default_wifi_sta();

        wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
        ESP_ERROR_CHECK(esp_wifi_init(&cfg));

        // Register Event Handlers
        ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, 
                        &wifi_event_handler, NULL, NULL));
        ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, 
                        &wifi_event_handler, NULL, NULL));

        wifi_initialized = true;
    }

    // 2. Configure the Wi-Fi credentials
    wifi_config_t wifi_config = {0};
    
    // Copy SSID
    strncpy((char *)wifi_config.sta.ssid, ssid, sizeof(wifi_config.sta.ssid));
    
    // Copy Password (if provided)
    if (password != NULL) {
        strncpy((char *)wifi_config.sta.password, password, sizeof(wifi_config.sta.password));
    }

    // Security settings
    if (password == NULL || strlen(password) == 0) {
        wifi_config.sta.threshold.authmode = WIFI_AUTH_OPEN;
    } else {
        wifi_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;
    }
    
    wifi_config.sta.pmf_cfg.capable = true;
    wifi_config.sta.pmf_cfg.required = false;

    // 3. Apply Configuration
    ESP_LOGI(TAG, "Setting WiFi configuration SSID: %s", ssid);
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));

    // 4. Start or Restart Connection
    // If wifi is already started, we disconnect to force the new config to take effect
    esp_wifi_stop(); 
    ESP_ERROR_CHECK(esp_wifi_start()); 
    // Note: esp_wifi_start() triggers WIFI_EVENT_STA_START, which calls esp_wifi_connect() in the handler.
}