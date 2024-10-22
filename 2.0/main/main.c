#include <stdio.h>
#include <string.h>
#include "esp_log.h"
#include "esp_wifi.h"
#include "nvs_flash.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_task_wdt.h"
#include "freertos/event_groups.h"
#include "my_utils.h"
#include "wifi_helper.h"
#include "mqtt_helper.h"
#include "espnow_helper.h"
#include "nvs_helper.h"


void app_main(void) {
    /*1. Initialize NVS*/
    ESP_ERROR_CHECK(nvs_helper_init());

    /*2. Start LWIP task*/
    ESP_ERROR_CHECK(esp_netif_init());

    /*3. Create default event loop*/
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    
    /*4. Initialize Wi-Fi*/
    wifi_helper_init();

    /*1. Send ESPNOW group messages*/
    vTaskDelay(10000 / portTICK_PERIOD_MS);

    while (1)
    {
        vTaskDelay(10000 / portTICK_PERIOD_MS);
    }


}

