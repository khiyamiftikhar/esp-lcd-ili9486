idf_component_register(SRCS "main.c" "panel_lvgl_init.c" "tests.c"
                    INCLUDE_DIRS "."
                    PRIV_REQUIRES esp-lcd-ili9486 lvgl esp_lvgl_port
                    REQUIRES esp_lcd driver)