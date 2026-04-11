#ifndef CONFIG_APP_H
#define CONFIG_APP_H

#include <stdint.h>
#include <stdbool.h>

#define MAX_SSID_LEN 32
#define MAX_PASS_LEN 64

// Struct to hold active configuration
typedef struct {
    char ssid[MAX_SSID_LEN];
    char password[MAX_PASS_LEN];
    int display_id;
    uint8_t theme_id; // 1: Pulsing, 2: BlackWhite, 3: WhiteBlack
} app_config_t;

// Global instance of the config
extern app_config_t g_app_config;

// Load config from NVS. Returns true if valid credentials exist.
bool config_app_load(void);

// Save current g_app_config to NVS
void config_app_save(void);

// Format/Reset config structure
void config_app_reset(void);

#endif // CONFIG_APP_H
