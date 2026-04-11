#include "config_app.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "esp_log.h"
#include <string.h>

static const char *TAG = "CONFIG";
static const char *NVS_NAMESPACE = "coffee_cfg";

app_config_t g_app_config;

void config_app_reset(void) {
    memset(&g_app_config, 0, sizeof(g_app_config));
    g_app_config.display_id = 1; // Default
    g_app_config.theme_id = 1;   // Default (Pulsing)
}

bool config_app_load(void) {
    nvs_handle_t my_handle;
    esp_err_t err;

    err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);

    config_app_reset(); // Set defaults first

    err = nvs_open(NVS_NAMESPACE, NVS_READONLY, &my_handle);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "Error opening NVS handle! Code: %s", esp_err_to_name(err));
        return false;
    }

    size_t required_size = 0;
    
    // Load SSID
    required_size = MAX_SSID_LEN;
    err = nvs_get_str(my_handle, "ssid", g_app_config.ssid, &required_size);
    if (err != ESP_OK) g_app_config.ssid[0] = '\0';

    // Load Password
    required_size = MAX_PASS_LEN;
    err = nvs_get_str(my_handle, "password", g_app_config.password, &required_size);
    if (err != ESP_OK) g_app_config.password[0] = '\0';

    // Load Display ID
    int32_t val;
    err = nvs_get_i32(my_handle, "display_id", &val);
    if (err == ESP_OK) g_app_config.display_id = (int)val;

    // Load Theme ID
    uint8_t theme_val;
    err = nvs_get_u8(my_handle, "theme_id", &theme_val);
    if (err == ESP_OK) g_app_config.theme_id = theme_val;

    nvs_close(my_handle);

    ESP_LOGI(TAG, "Config Loaded: SSID='%s', ID=%d, Theme=%d", g_app_config.ssid, g_app_config.display_id, g_app_config.theme_id);

    return (strlen(g_app_config.ssid) > 0);
}

void config_app_save(void) {
    nvs_handle_t my_handle;
    esp_err_t err;

    err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &my_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error opening NVS to save! Code: %s", esp_err_to_name(err));
        return;
    }

    nvs_set_str(my_handle, "ssid", g_app_config.ssid);
    nvs_set_str(my_handle, "password", g_app_config.password);
    nvs_set_i32(my_handle, "display_id", (int32_t)g_app_config.display_id);
    nvs_set_u8(my_handle, "theme_id", g_app_config.theme_id);

    err = nvs_commit(my_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "NVS Commit failed! %s", esp_err_to_name(err));
    } else {
        ESP_LOGI(TAG, "Config Saved successfully");
    }

    nvs_close(my_handle);
}
