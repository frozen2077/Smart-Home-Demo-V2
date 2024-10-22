#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "esp_log.h"
#include "esp_mac.h"
#include "esp_task_wdt.h"
#include "nvs_flash.h"

#include "esp_wifi.h"
#include "lwip/inet.h"
#include "esp_http_server.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

#include "cJSON.h"
#include "dns_server.h"
#include "my_utils.h"
#include "ws_helper.h"
#include "mqtt_helper.h"
#include "nvs_helper.h"
#include "espnow_helper.h"

#define DEFAULT_START_MODE_STA      0

#define DEFAULT_AP_WIFI_SSID        "SoftAP"
#define DEFAULT_AP_WIFI_PASS        ""
#define DEFAULT_AP_WIFI_CHANNEL     (1)

#define DEFAULT_STA_WIFI_SSID       "@ZHANGSU_2.4G"
#define DEFAULT_STA_WIFI_PASS       "zsz19980829"

#define WIFI_INFO_MAX_LENGTH        (20)
#define HTTP_SERVER_MAX_CLIENT      (4)

//----------------------------------------------------

#define WIFI_STA_START_BIT          BIT0
#define WIFI_STA_CONNECTED_BIT      BIT1
#define WIFI_STA_DISCONNECTED_BIT   BIT2

#define WIFI_APSTA_START_BIT        BIT0

#define WIFI_SCAN_SCANNING_BIT      BIT0
#define WIFI_SCAN_DONE_BIT          BIT1

#define WIFI_HELPER_BASE_PATH       ""

#define WIFI_SCAN_RUNNING           (-1)

#define WS_ASYNC_SEND_MAX_QUEUE_TIME  (5000 / portTICK_PERIOD_MS)

extern const char index_start[]       asm("_binary_index_html_start");
extern const char index_end[]         asm("_binary_index_html_end");
extern const char favicon_ico_start[] asm("_binary_favicon_ico_start");
extern const char favicon_ico_end[]   asm("_binary_favicon_ico_end");

typedef struct {
    httpd_handle_t handle;
    int socketd;
    char payload[800];
} async_resp_arg_t;

typedef struct {
    char ssid[32];                         /**< SSID of target AP. */
    char password[32];                     /**< Password of target AP. */
} my_wifi_info_t;

esp_err_t wifi_helper_init(void);

