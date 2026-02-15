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
)

