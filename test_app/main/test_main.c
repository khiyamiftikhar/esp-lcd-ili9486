/**
 * app_main.c — ili9486 test_app entry point
 *
 * Initializes hardware then runs all Unity test cases from test_panel.c.
 *
 * Build and flash:
 *   idf.py -C test_app build flash monitor
 *
 * Run specific tag only:
 *   Unity picks up TEST_CASE("[ili9486]") automatically.
 *   In monitor, send 'ili9486' to filter by tag.
 */

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "unity.h"
//#include "test_ili9486_panel.h"

static const char *TAG = "test_app";

void app_main(void)
{
    ESP_LOGI(TAG, "ILI9486 panel driver test app");

    UNITY_BEGIN();
    unity_run_all_tests();
    UNITY_END();

    /*
     * After all tests, sit in the monitor loop so you can
     * re-run individual tests by name via serial input.
     * Send test name e.g. "single pixel at origin" to re-run one.
     * Send "all" to re-run everything.
     * Ctrl+] to exit monitor.
     */
    unity_run_menu();
}