idf_component_register(SRCS "test_esp_ili9486_panel.c" "panel_init.c"
                    INCLUDE_DIRS "."
                    PRIV_REQUIRES esp-lcd-ili9486 unity)