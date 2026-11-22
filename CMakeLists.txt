idf_component_register(SRCS "src/esp_ili9486_panel.c"
                    INCLUDE_DIRS "include"
                    REQUIRES esp_lcd
                    PRIV_REQUIRES esp_driver_gpio)