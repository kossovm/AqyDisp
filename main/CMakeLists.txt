# Embed the server root certificate into the final binary
idf_build_get_property(project_dir PROJECT_DIR)

idf_component_register(
    SRC_DIRS
        "."                 # main.c lives here
        "GUI"               # ui.c, ui_events.c, helpers, etc.
        "GUI/components"
        "GUI/screens"       # all screen .c files *with* extension
        "GUI/images"        # auto-generated image sources
        "GUI/fonts"         # auto-generated font sources
        "LVGL_init"
        "OTA"
        "Sensors"
        "WIFI"
        "JSON"

    INCLUDE_DIRS "."
    REQUIRES
        cjson
        esp_event
        esp_netif
        esp_http_client
        esp_https_ota
        nvs_flash
        esp_wifi
        esp_lcd
        esp_timer
        app_update
        esp_driver_ledc
        esp_driver_gpio
        esp_driver_spi
        esp_driver_i2c
        esp_adc
        mbedtls
)

