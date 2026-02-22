idf_component_register(SRCS "main.c" "panel_init.c" "tests.c"
                    INCLUDE_DIRS "."
                    PRIV_REQUIRES esp-lcd-ili9486 
                    REQUIRES esp_lcd driver)