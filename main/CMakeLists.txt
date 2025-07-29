# Embed the server root certificate into the final binary
idf_build_get_property(project_dir PROJECT_DIR)

idf_component_register(
    SRCS           "main.c"
                   "GUI/ui.c"
                   "GUI/ui_events.c"
                   "GUI/components/ui_comp_hook.c"
                   "GUI/screens/ui_Screen1.c"
                   "GUI/ui_helpers.c"
                   "LVGL_init/aqylab.c"
                   "OTA/ota.c"
                   "Sensors/sensors.c"
                   
    
    INCLUDE_DIRS    "."
    
    EMBED_TXTFILES ${project_dir}/server_certs/ca_cert.pem
    
)

