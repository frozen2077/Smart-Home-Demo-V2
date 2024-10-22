#include "wifi_helper.h"
#include "my_rtos.h"

static const char * TAG = "WIFI_HELPER";

SemaphoreHandle_t            wifi_connect_lock                = NULL;

static TaskHandle_t          _wifi_sta_task_handle            = NULL;
static TaskHandle_t          _wifi_apsta_task_handle          = NULL;
static TaskHandle_t          _wifi_scan_task_handle           = NULL;
static TaskHandle_t          _wifi_ws_async_send_task_handle  = NULL;

static QueueHandle_t         _ws_async_msg_queue              = NULL;

static EventGroupHandle_t    _wifi_apsta_event_group          = NULL;
static EventGroupHandle_t    _wifi_sta_event_group            = NULL;
static EventGroupHandle_t    _wifi_scan_event_group           = NULL;

static uint32_t              _scanStarted                     = 0;
static bool                  _scanDone                        = false;
static bool                  _staStarted                      = false;
static bool                  _staConnected                    = false;
static bool                  _apstaStarted                    = false;
static bool                  _webServerStarted                = false;

static nvs_handle_t          nvs_wifi_handle;

static esp_netif_t           *esp_netif_sta, *esp_netif_ap    = NULL;

static httpd_handle_t        server                           = NULL;
static httpd_req_t           *httpd_req_ws_p                  = NULL;

static const char            *base_path                       = WIFI_HELPER_BASE_PATH;

static int                   ws_clients[4]                    = {-1};

static esp_err_t             wifi_helper_start_scan();
static esp_err_t             wifi_helper_start_apsta();
static esp_err_t             wifi_helper_start_sta(my_wifi_info_t my_wifi_info);
static char                  *_wifi_mode_to_name(wifi_mode_t* mode); 
static const char            *_wifi_event_to_string(wifi_event_t* event); 


static void wifi_event_handler(void *handler_arg, 
                               esp_event_base_t base, int32_t id, void *event_data) 
{
    const char * TAG = "WIFI_EVENT_HANDLE";
    wifi_mode_t mode;
    esp_wifi_get_mode(&mode);
    ESP_LOGI(TAG, "[%s] | Mode: %s", _wifi_event_to_string(&id), _wifi_mode_to_name(&mode));

    switch (id) {
        /*STA Event -----------------------------------------------------------*/            
        case WIFI_EVENT_STA_START:
            if (mode == WIFI_MODE_STA) {
                xEventGroupSetBits(_wifi_sta_event_group, WIFI_STA_START_BIT);}
            break;
        case WIFI_EVENT_STA_DISCONNECTED:
            if (mode == WIFI_MODE_STA) {
                xEventGroupSetBits(_wifi_sta_event_group, WIFI_STA_DISCONNECTED_BIT);}
            break;
        /*SCAN Event -----------------------------------------------------------*/              
        case WIFI_EVENT_SCAN_DONE:
            xEventGroupSetBits(_wifi_scan_event_group, WIFI_SCAN_DONE_BIT);
            break;
        /*AP Event -----------------------------------------------------------*/    
        case WIFI_EVENT_AP_START:
            if (mode == WIFI_MODE_APSTA)
                xEventGroupSetBits(_wifi_apsta_event_group, WIFI_APSTA_START_BIT);
            break;        
        case WIFI_EVENT_AP_STACONNECTED:
            wifi_event_ap_staconnected_t* event_c = 
                                    (wifi_event_ap_staconnected_t*) event_data;
            ESP_LOGI(TAG, "station "MACSTR" join, AID=%d",
                    MAC2STR(event_c->mac), event_c->aid);        
            break;   
        case WIFI_EVENT_AP_STADISCONNECTED:
            wifi_event_ap_stadisconnected_t* event_nc = 
                                    (wifi_event_ap_stadisconnected_t*) event_data;
            ESP_LOGI(TAG, "station "MACSTR" leave, AID=%d",
                    MAC2STR(event_nc->mac), event_nc->aid);          
        default:
            break;
    } 
}


static void ip_event_handler(void *handler_arg, esp_event_base_t base, 
                             int32_t id, void *event_data) 
{
    const char * TAG = "IP_EVENT_HANDLE";
    ESP_LOGI(TAG, "[%ld]", id);

    switch (id) {
        case IP_EVENT_STA_GOT_IP:
            ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
            ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
            xEventGroupSetBits(_wifi_sta_event_group, WIFI_STA_CONNECTED_BIT);
    } 
}

static void send_number(void *arg)
{
    static uint8_t num = 0;   
    static char data[20] = {0};

    memset(data , 0, sizeof(data));
    snprintf(data, sizeof(data), "This is %d", num++);

    async_resp_arg_t *resp_arg = arg;
    httpd_handle_t hd = resp_arg->handle;
    int fd = resp_arg->socketd;

    httpd_ws_frame_t ws_pkt;
    memset(&ws_pkt, 0, sizeof(httpd_ws_frame_t));

    ws_pkt.payload = (uint8_t*)data;
    ws_pkt.len = strlen(data);
    ws_pkt.type = HTTPD_WS_TYPE_TEXT;

    httpd_ws_send_frame_async(hd, fd, &ws_pkt);
    free(resp_arg);
}

static void send_ping(void *arg)
{
    async_resp_arg_t *resp_arg = arg;
    httpd_handle_t hd = resp_arg->handle;
    int fd = resp_arg->socketd;
    httpd_ws_frame_t ws_pkt;
    memset(&ws_pkt, 0, sizeof(httpd_ws_frame_t));
    ws_pkt.payload = NULL;
    ws_pkt.len = 0;
    ws_pkt.type = HTTPD_WS_TYPE_PING;

    httpd_ws_send_frame_async(hd, fd, &ws_pkt);
    free(resp_arg);
}

static void _async_send(void *arg)
{  
    async_resp_arg_t *resp_arg = arg;
    httpd_handle_t hd = resp_arg->handle;
    int fd = resp_arg->socketd;
    // char * data = resp_arg->payload;

    httpd_ws_frame_t ws_pkt;
    memset(&ws_pkt, 0, sizeof(httpd_ws_frame_t));

    ws_pkt.payload = (uint8_t*)resp_arg->payload;
    ws_pkt.len = strlen(resp_arg->payload);
    ws_pkt.type = HTTPD_WS_TYPE_TEXT;

    ESP_LOGI("WS ASYNC SEND", "Async message is: %s", resp_arg->payload);
    ESP_LOG_INFO(httpd_ws_send_frame_async(hd, fd, &ws_pkt), "Async message sent failed");

    free(resp_arg);
}


static esp_err_t ws_async_send_task(void* pv)
{
    const char * TAG = "WS_ASYNC_SEND_TASK";
    size_t clients = HTTP_SERVER_MAX_CLIENT;
    int client_fds[HTTP_SERVER_MAX_CLIENT];
    static char payload[800];

    while(true) {
        if (!server) {
            vTaskDelay(pdMS_TO_TICKS(100));
            continue;
        }

        if (xQueueReceive(_ws_async_msg_queue, payload, portMAX_DELAY) == pdPASS) {
            esp_err_t ret = httpd_get_client_list(server, &clients, client_fds);
            ESP_ERROR_CONTINUE(ret != ESP_OK,  "Failed to get client list");

            for (size_t i = 0; i < clients; ++i) {
                int sock = client_fds[i];
                if (httpd_ws_get_fd_info(server, sock) != HTTPD_WS_CLIENT_WEBSOCKET) {
                    continue;
                }

                ESP_LOGI(TAG, "Sending async message to active client (fd=%d)", sock);
                async_resp_arg_t *resp_arg = malloc(sizeof(async_resp_arg_t));
                ESP_ERROR_RETURN(resp_arg == NULL, ESP_ERR_NO_MEM, "Memory allocation failed");

                resp_arg->handle = server;
                resp_arg->socketd = sock;
                memset(resp_arg->payload, 0, sizeof(resp_arg->payload));
                strncpy(resp_arg->payload, payload, sizeof(resp_arg->payload) - 1);

                if (httpd_queue_work(resp_arg->handle, _async_send, resp_arg) != ESP_OK) {
                                       ESP_LOGE(TAG, "Failed to queue work for async send");
                    free(resp_arg);
                    continue;
                }

                ESP_LOGI(TAG, "Successfully queued async message for client (fd=%d)", sock);
            }
        }
    }
    vTaskDelete(NULL);
}

static esp_err_t ws_async_send(char *payload) 
{
    esp_err_t ret = ESP_OK;
    const char * TAG = "WS Async send";

    if (_wifi_ws_async_send_task_handle == NULL) {
        return ESP_FAIL;
    }

    if (xQueueSend(_ws_async_msg_queue, payload, WS_ASYNC_SEND_MAX_QUEUE_TIME) != pdPASS)  {
        ESP_LOGI(TAG, "Payload sent Failed");    
        ret = ESP_FAIL;     
    } else {
        ESP_LOGI(TAG, "Payload sent Successfully");
    }

    return ret;
}


bool client_not_alive_cb(ws_helper_t h, int fd)
{
    ESP_LOGE(TAG, "Client not alive, closing fd %d", fd);
    httpd_sess_trigger_close(ws_helper_get_user_ctx(h), fd);
    return true;
}

bool check_client_alive_cb(ws_helper_t h, int fd)
{
    ESP_LOGD(TAG, "Checking if client (fd=%d) is alive", fd);
    async_resp_arg_t *resp_arg = malloc(sizeof(async_resp_arg_t));
    assert(resp_arg != NULL);
    resp_arg->handle = ws_helper_get_user_ctx(h);
    resp_arg->socketd = fd;

    if (httpd_queue_work(resp_arg->handle, send_ping, resp_arg) == ESP_OK) {
        return true;
    }
    return false;
}

static const char* get_path_from_uri(char *dest, const char *base_path, const char *uri, size_t destsize)
{
    const size_t base_pathlen = strlen(base_path);
    size_t pathlen = strlen(uri);

    const char *quest = strchr(uri, '?');
    if (quest) {
        pathlen = MIN(pathlen, quest - uri);
    }
    const char *hash = strchr(uri, '#');
    if (hash) {
        pathlen = MIN(pathlen, hash - uri);
    }
    if (base_pathlen + pathlen + 1 > destsize) {
        /* Full path string won't fit into destination buffer */
        return NULL;
    }
    /* Construct full path (base + path) */
    strcpy(dest, base_path);
    strlcpy(dest + base_pathlen, uri, pathlen + 1);

    /* Return pointer to path, skipping the base */
    return dest + base_pathlen;
}

esp_err_t uri_handler_404(httpd_req_t *req, httpd_err_code_t err)
{
    const char * TAG = "Handler 404";
    httpd_resp_set_status(req, "302 Temporary Redirect");
    httpd_resp_set_hdr(req, "Location", "/");
    httpd_resp_send(req, "Redirect to the captive portal", HTTPD_RESP_USE_STRLEN);

    ESP_LOGI(TAG, "Redirecting to /");
    return ESP_OK;
}

static esp_err_t _ws_handle_text_message(char *buf)
{
    const char * TAG = "WS_TEXT_HANDLER";

    cJSON *root = cJSON_Parse(buf);
    ESP_ERROR_RETURN(root == NULL, ESP_FAIL, "Failed to parse JSON");

    cJSON *v_t = cJSON_GetObjectItem(root, "t");
    ESP_ERROR_GOTO(!cJSON_IsString(v_t) || v_t->valuestring == NULL,
                    FAIL, "Invalid or missing 't' field");

    ESP_LOGI(TAG, "T String is: %s", v_t->valuestring);

    if (strcmp(v_t->valuestring, "cmd") == 0) {
        cJSON *v_c = cJSON_GetObjectItem(root, "c");
        ESP_ERROR_GOTO(!cJSON_IsString(v_c) || v_c->valuestring == NULL,
                        FAIL, "Invalid or missing 'c' field");

        ESP_LOGI(TAG, "C String is: %s", v_c->valuestring);

        if (strcmp(v_c->valuestring, "scan") == 0) {
            wifi_helper_start_scan();
        } 

        if (strcmp(v_c->valuestring, "setgroup") == 0) {
            uint8_t addrs_list[][6] = {
                {0xc0, 0x49, 0xef, 0xcd, 0xde, 0xdc}
            };
            uint8_t group_id[6] = {
                {0x01, 0x02, 0x03, 0x04, 0x05, 0x06}
            };    
            ESP_LOGI("MAIN", "Sending ESPNOW group messages");
            espnow_set_group(addrs_list, 1, group_id, true);  
        } 

        if (strcmp(v_c->valuestring, "con") == 0) {
            cJSON *v_s = cJSON_GetObjectItem(root, "ssid");
            cJSON *v_p = cJSON_GetObjectItem(root, "pass");
            if (cJSON_IsString(v_s) && cJSON_IsString(v_p) && v_s->valuestring != NULL && v_p->valuestring != NULL) {
                my_wifi_info_t* my_wifi_info = (my_wifi_info_t*)calloc(1, sizeof(my_wifi_info_t));
                strcpy(my_wifi_info->ssid, v_s->valuestring);
                strcpy(my_wifi_info->password, v_p->valuestring);
                wifi_helper_start_sta(*my_wifi_info);
                free(my_wifi_info);
            } else {
                ESP_LOGE(TAG, "Invalid or missing 'ssid' or 'pass' field");
                goto FAIL;
            }
        }
    }

    return ESP_OK;

FAIL:
    cJSON_Delete(root);
    return ESP_FAIL;
}

static esp_err_t uri_handler_ws(httpd_req_t *req)
{
    esp_err_t ret = ESP_OK;
    httpd_ws_frame_t ws_pkt = {0};
    char *buf = NULL;

    if (req->method == HTTP_GET) {
        ESP_LOGI(TAG, "Handshake done, the new connection was opened");
        return ESP_OK;
    }

    ret = httpd_ws_recv_frame(req, &ws_pkt, 0);
    ESP_ERROR_RETURN(ret != ESP_OK, ret, "Receiving WebSocket frame failed");


    if (ws_pkt.len) {
        buf = calloc(1, ws_pkt.len + 1); // Allocate memory for the buffer plus NULL terminator
        ESP_ERROR_RETURN(!buf, ESP_ERR_NO_MEM, "Failed to allocate memory for WebSocket packet");

        ws_pkt.payload = (uint8_t *)buf;

        ret = httpd_ws_recv_frame(req, &ws_pkt, ws_pkt.len);
        ESP_ERROR_GOTO(ret != ESP_OK, EXIT, "Receiving WebSocket frame payload failed");

        ESP_LOGI(TAG, "Received packet with message: %s", ws_pkt.payload);
    }

    switch (ws_pkt.type) {
        case HTTPD_WS_TYPE_PONG:
            ESP_LOGD(TAG, "Received PONG message");
            ret = ws_helper_client_is_active(server, httpd_req_to_sockfd(req));
            return ret;
        case HTTPD_WS_TYPE_TEXT:
            ESP_LOGI(TAG, "Received WS TEXT message: %s", buf);
            ret = _ws_handle_text_message(buf);
            break;
        case HTTPD_WS_TYPE_PING:
            ESP_LOGI(TAG, "Received PING, sending PONG");
            ws_pkt.type = HTTPD_WS_TYPE_PONG;
            ret = httpd_ws_send_frame(req, &ws_pkt);
            ESP_ERROR_GOTO(ret != ESP_OK, EXIT, "Sending WebSocket Ping failed");
            break;
        default:
            ESP_LOGI(TAG, "Received unsupported WebSocket frame type: %d", ws_pkt.type);
            break;
    }

    ESP_LOGI(TAG, "WebSocket handler: httpd_handle_t=%p, sockfd=%d, client_info:%d", req->handle,
             httpd_req_to_sockfd(req), httpd_ws_get_fd_info(req->handle, httpd_req_to_sockfd(req)));

EXIT:
    free(buf);
    return ret;
}

static esp_err_t uri_handler_icon(httpd_req_t *req)
{
    const size_t favicon_ico_size = (favicon_ico_end - favicon_ico_start);
    httpd_resp_set_type(req, "image/x-icon");
    httpd_resp_send(req, (const char *)favicon_ico_start, favicon_ico_size);
    return ESP_OK;
}

static esp_err_t uri_handler_index(httpd_req_t *req)
{
    const char * TAG = "Handler [Index]";

    char filepath[20];
    struct stat file_stat;

    const char *filename = get_path_from_uri(filepath, base_path,
                                             req->uri, sizeof(filepath));
    if (!filename) {
        ESP_LOGE(TAG, "Filename is too long");
        /* Respond with 500 Internal Server Error */
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Filename too long");
        return ESP_FAIL;
    }

    if (filename[strlen(filename) - 1] == '/') {
        ESP_LOGI(TAG, "Serveing index.html");
        httpd_resp_set_type(req, "text/html");
        const uint32_t index_len = index_end - index_start;
        httpd_resp_send(req, index_start, index_len);
        return ESP_OK;
    }

    return ESP_OK;
}

static esp_err_t uri_handler_scan(httpd_req_t *req)
{

    ESP_LOGI("scan_handler_index", "Scanning");

    ESP_LOGI(TAG, "Serveing index.html");
    httpd_resp_set_type(req, "text/html");
    const uint32_t index_len = index_end - index_start;
    httpd_resp_send(req, index_start, index_len);

    wifi_helper_start_scan();

    return ESP_OK;
}

esp_err_t sock_open_fd(httpd_handle_t hd, int sockfd)
{
    if (httpd_ws_get_fd_info(hd, sockfd) == HTTPD_WS_CLIENT_WEBSOCKET) {
        ESP_LOGI(TAG, "New WS client connected %d", sockfd);
        ws_helper_t h = httpd_get_global_user_ctx(hd);
        return ws_helper_add_client(h, sockfd);
    }
    return ESP_OK;
}

void sock_close_fd(httpd_handle_t hd, int sockfd)
{
    ESP_LOGI(TAG, "Client disconnected %d", sockfd);
    ws_helper_t h = httpd_get_global_user_ctx(hd);
    ws_helper_remove_client(h, sockfd);
    close(sockfd);
}

bool ws_client_not_alive_cb(ws_helper_t h, int fd)
{
    ESP_LOGE(TAG, "Client not alive, closing fd %d", fd);
    httpd_sess_trigger_close(ws_helper_get_user_ctx(h), fd);
    return true;
}

bool ws_check_client_alive_cb(ws_helper_t h, int fd)
{
    ESP_LOGD(TAG, "Checking if client (fd=%d) is alive", fd);
    async_resp_arg_t *resp_arg = malloc(sizeof(async_resp_arg_t));
    assert(resp_arg != NULL);
    resp_arg->handle = ws_helper_get_user_ctx(h);
    resp_arg->socketd = fd;

    if (httpd_queue_work(resp_arg->handle, send_ping, resp_arg) == ESP_OK) {
        return true;
    }
    return false;
}

static esp_err_t wifi_helper_start_webserver()
{
    // Prepare keep-alive engine
    ws_helper_config_t keep_alive_config    = KEEP_ALIVE_CONFIG_DEFAULT();
    keep_alive_config.max_clients           = HTTP_SERVER_MAX_CLIENT;
    keep_alive_config.client_not_alive_cb   = ws_client_not_alive_cb;
    keep_alive_config.check_client_alive_cb = ws_check_client_alive_cb;
    ws_helper_t keep_alive = ws_helper_start(&keep_alive_config);

    httpd_config_t config   = HTTPD_DEFAULT_CONFIG();
    // config.max_open_sockets = HTTP_SERVER_MAX_CLIENT;
    config.global_user_ctx  = keep_alive;
    config.open_fn          = sock_open_fd;
    config.close_fn         = sock_close_fd;    
    config.lru_purge_enable = true;
    config.uri_match_fn     = httpd_uri_match_wildcard;

    // Start the httpd server
    ESP_LOGI(TAG, "Starting server on port: '%d'", config.server_port);
    if (httpd_start(&server, &config) == ESP_OK) {
        _webServerStarted = true;
    } else {
        _webServerStarted = false;
        ESP_LOGE(TAG, "Failed to start file server!");
        return ESP_FAIL;
    }

    ws_helper_set_user_ctx(keep_alive, server);

    // Set URI handlers
    ESP_LOGI(TAG, "Register URI handlers");
    const httpd_uri_t icon_index = {
        .uri          = "/favicon.ico",
        .method       = HTTP_GET,
        .handler      = uri_handler_icon
    };
    const httpd_uri_t scan_index = {
        .uri          = "/api/scan",
        .method       = HTTP_GET,
        .handler      = uri_handler_scan
    }; 
    const httpd_uri_t uri_index = {
        .uri          = "/",
        .method       = HTTP_GET,
        .handler      = uri_handler_index
    };
    const httpd_uri_t ws_index = {
        .uri          = "/socket/ws",
        .method       = HTTP_GET,
        .handler      = uri_handler_ws,
        .user_ctx     = NULL,
        .is_websocket = true,
        .handle_ws_control_frames = true
    };
    httpd_register_uri_handler(server, &icon_index); 
    httpd_register_uri_handler(server, &scan_index); 
    httpd_register_uri_handler(server, &ws_index);
    httpd_register_uri_handler(server, &uri_index);       
    httpd_register_err_handler(server, HTTPD_404_NOT_FOUND, uri_handler_404);

    if (_wifi_ws_async_send_task_handle == NULL) {  
        xTaskCreate(ws_async_send_task, "WS Async send Task", 
                    1024 * 6, 
                    NULL, 
                    1, &_wifi_ws_async_send_task_handle);    
    }

    return ESP_OK;
}

static esp_err_t wifi_helper_stop_webserver()
{
    // Stop keep alive thread
    ws_helper_stop(httpd_get_global_user_ctx(server));
    // Stop the httpd server
    return httpd_stop(server);
}


void wifi_helper_apsta_task(void *pv) 
{
    const char * TAG = "WIFI_APSTA_TASK";

    while (1) {
        EventBits_t bits = xEventGroupWaitBits(_wifi_apsta_event_group,
                                                WIFI_APSTA_START_BIT,
                                                pdFALSE,
                                                pdFALSE,
                                                portMAX_DELAY);
        char bit_string[33];        
        itoa(bits, bit_string, 2);
        ESP_LOGI(TAG, "Eventgroup bits update: 0b%s", bit_string); 
        
        if (bits & WIFI_APSTA_START_BIT) {
            xEventGroupClearBits(_wifi_apsta_event_group, WIFI_APSTA_START_BIT);
            _apstaStarted = true;
            /*1. Stop ESPNOW*/
            ESP_LOG_INFO(espnow_stop(), "Stop ESPNOW Failed");

            /*2. Start Webserver*/
            ESP_LOGI(TAG, "Connection established, starting web server."); 
            if (server == NULL) {
                wifi_helper_start_webserver(server);
            }    
            // dns_server_config_t config = DNS_SERVER_CONFIG_SINGLE("*", "WIFI_AP_DEF");
            // start_dns_server(&config);            
        } 
    }    
}

esp_err_t wifi_helper_save_wifi_info() 
{
    const char *TAG = "WIFI_SAVE_INFO";
    esp_err_t ret = ESP_OK;

    my_wifi_info_t* my_wifi_info = (my_wifi_info_t*)calloc(1, sizeof(my_wifi_info_t));
    esp_err_t ret1 = nvs_helper_blob_get("mywifi", "my_wifi_info", my_wifi_info, sizeof(my_wifi_info_t));

    wifi_config_t sta_config;
    ret = esp_wifi_get_config(WIFI_IF_STA, &sta_config);
    ESP_ERROR_RETURN(ret != ESP_OK, ESP_FAIL, "Failed to get wifi info from STA config");
    char *ssid = (char *)sta_config.sta.ssid;
    char *pass = (char *)sta_config.sta.password;

    ESP_LOGI(TAG, "Wifi Current ssid: %s, password: %s", ssid , pass);

    ESP_ERROR_RETURN(strlen(ssid) == 0, ESP_FAIL, "Wifi SSID is empty");

    if (ret1 == ESP_OK) {
        ESP_LOGI(TAG, "Wifi NVS ssid: %s, password: %s", my_wifi_info->ssid , my_wifi_info->password);
        if(strcmp(my_wifi_info->ssid, ssid) == 0 && strcmp(my_wifi_info->password, pass) == 0) {
            ESP_LOGI(TAG, "Wifi info no need to save");
            return ESP_OK;
        }
    } else {
        ESP_LOGW(TAG, "Failed to get info from NVS");
    }      

    my_wifi_info_t* new_wifi_info = (my_wifi_info_t*)calloc(1, sizeof(my_wifi_info_t));
    memcpy(new_wifi_info->ssid, ssid, strlen(ssid));
    memcpy(new_wifi_info->password, pass, strlen(pass));
    ret = nvs_helper_blob_set("mywifi", "my_wifi_info", new_wifi_info, sizeof(my_wifi_info_t));

    ESP_ERROR_RETURN(ret != ESP_OK, ESP_FAIL, "Wifi info failed saved to NVS");

    free(my_wifi_info);
    free(new_wifi_info);
    ESP_LOGI(TAG, "Wifi info saved to NVS");

    return ret;

}

void _start_apsta_task(void *pv){
    wifi_helper_start_apsta();
    vTaskDelete(NULL);
}

void wifi_helper_sta_task(void *pv) 
{
    const char * TAG = "WIFI_STA_TASK";
    static uint8_t _retry_num = 0;

    while (1) {
        EventBits_t bits = xEventGroupWaitBits(_wifi_sta_event_group,
                                                0b11111111,
                                                pdFALSE,
                                                pdFALSE,
                                                portMAX_DELAY);
        char bit_string[33];        
        itoa(bits, bit_string, 2);
        ESP_LOGI(TAG, "Eventgroup bits update: 0b%s", bit_string); 

        if (bits & WIFI_STA_START_BIT) {
            xEventGroupClearBits(_wifi_sta_event_group, WIFI_STA_START_BIT);
            ESP_ERROR_CHECK(esp_wifi_connect());

        } 
        else if (bits & WIFI_STA_CONNECTED_BIT) {
            xEventGroupClearBits(_wifi_sta_event_group, WIFI_STA_CONNECTED_BIT); 
            _retry_num = 0;
            _staConnected = true;
            /*1. Save wifi info to NVS*/
            wifi_helper_save_wifi_info();

            /*2. Start HTTPD Server*/
            if (server == NULL) {
                ESP_LOGI(TAG, "Connection established, starting web server."); 
                wifi_helper_start_webserver(server);   
            } 
            
            /*3. Start MQTT Client*/
            esp_mqtt_client_config_t mqtt_cfg = {
                .broker.address.uri = "mqtt://192.168.6.248:1883",
            };
            mqtt_helper_start(mqtt_cfg);

            // const esp_mqtt_topic_t tpc[] = {{MQTT_TOPIC_SEND_CMD, 1}};
            // size_t len = sizeof(tpc) / sizeof(esp_mqtt_topic_t);
            // ESP_LOG_INFO(mqtt_subscribe_from_list(TOPIC_BASEPATH, tpc, len), 
            //             "mqtt sub failed");

            // ESP_LOG_INFO(mqtt_enqueue_from_list(TOPIC_BASEPATH, tpc, len, "hello every body", 0), 
            //             "mqtt send failed");

            /*4. Start ESPNOW*/            
            espnow_config_t espnow_config = ESPNOW_INIT_CONFIG_DEFAULT();
            espnow_config.master_channel = true;
            espnow_start(&espnow_config);

        } 
        else if (bits & WIFI_STA_DISCONNECTED_BIT) {
            xEventGroupClearBits(_wifi_sta_event_group, WIFI_STA_DISCONNECTED_BIT);
            _staConnected = false;
            /*1. Stop MQTT Client*/
            mqtt_helper_stop();
            /*2. Retry connecting*/
            if (xSemaphoreTake(wifi_connect_lock, pdMS_TO_TICKS(5000)) == pdTRUE) {
                if (_retry_num < 1) {
                    _retry_num++;
                    esp_wifi_connect();
                    ESP_LOGI(TAG, "Retry connecting: %d times", _retry_num); 
                } 
                /*3. Start APSTA Mode*/
                else {
                    ESP_LOGW(TAG, "Failed to connect to WIFI, starting APSTA Mode");
                    /*1. Restart in apsta mode*/
                    xTaskCreatePinnedToCore(_start_apsta_task, "Wifi APSTA Task", 
                                1024 * 4, 
                                NULL, 
                                1, NULL, 0);
                }
                xSemaphoreGive(wifi_connect_lock);
            } else {
                ESP_LOGW(TAG, "Failed to take wifi connect lock");
            }
        } 
    }
}


/* Initialize soft AP */
esp_netif_t *wifi_init_softap(void)
{
    const char * TAG = "WIFI_AP_INIT";

    if (esp_netif_ap == NULL)
        esp_netif_ap = esp_netif_create_default_wifi_ap();

    esp_netif_ip_info_t ip_info;
    IP4_ADDR(&ip_info.ip, 4, 3, 2, 1);
    IP4_ADDR(&ip_info.gw, 4, 3, 2, 1);
    IP4_ADDR(&ip_info.netmask, 255, 255, 255, 0);

    esp_netif_dhcps_stop(esp_netif_ap);
    esp_netif_set_ip_info(esp_netif_ap, &ip_info);
    esp_netif_dhcps_start(esp_netif_ap);

    wifi_config_t wifi_ap_config = {
        .ap = {
            .ssid     = DEFAULT_AP_WIFI_SSID,
            .ssid_len = strlen(DEFAULT_AP_WIFI_SSID),
            .channel  = DEFAULT_AP_WIFI_CHANNEL,
            .password = DEFAULT_AP_WIFI_PASS,
            .max_connection = 12,
            .authmode = WIFI_AUTH_WPA2_PSK,
        },
    };
    if (strlen(DEFAULT_AP_WIFI_PASS) == 0) {
        wifi_ap_config.ap.authmode = WIFI_AUTH_OPEN;
    }

    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_ap_config));

    ESP_LOGI(TAG, "wifi_init_softap finished. SSID:%s password:%s channel:%d",
             DEFAULT_AP_WIFI_SSID, DEFAULT_AP_WIFI_PASS, DEFAULT_AP_WIFI_CHANNEL);

    return esp_netif_ap;
}

/* Initialize wifi station */
esp_netif_t *wifi_init_sta(my_wifi_info_t my_wifi_info)
{
    const char * TAG = "WIFI_STA_INIT";

    if (esp_netif_sta == NULL)
        esp_netif_sta = esp_netif_create_default_wifi_sta();

    wifi_config_t wifi_sta_config = {
        .sta = {           
            .scan_method = WIFI_ALL_CHANNEL_SCAN,
            .failure_retry_cnt = 3,
            .threshold.authmode = WIFI_AUTH_WPA2_PSK,
            .sae_pwe_h2e = WPA3_SAE_PWE_BOTH,           
        },
    };

    const char *ssid = my_wifi_info.ssid;
    const char *pwd  = my_wifi_info.password;
    ESP_LOGI(TAG, "ssid is %s, pwd is %s", ssid, pwd);

    if (strlen(ssid) > 0) {
        memcpy(wifi_sta_config.sta.ssid, ssid, strlen(ssid));
        memcpy(wifi_sta_config.sta.password, pwd, strlen(pwd));
    } else {
        memcpy(wifi_sta_config.sta.ssid, DEFAULT_STA_WIFI_SSID, strlen(DEFAULT_STA_WIFI_SSID));
        memcpy(wifi_sta_config.sta.password, DEFAULT_STA_WIFI_PASS, strlen(DEFAULT_STA_WIFI_PASS));        
    }        

    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_sta_config) );

    ESP_LOGI(TAG, "wifi_init_sta finished.");

    return esp_netif_sta;
}

esp_err_t wifi_helper_start_apsta(void) {

    esp_err_t ret = ESP_OK;
    const char * TAG = "WIFI_APSTA_START";

    if( _wifi_apsta_task_handle == NULL )
    xTaskCreate(wifi_helper_apsta_task, "Wifi AP Task", 
                1024 * 6, 
                NULL, 
                1, &_wifi_apsta_task_handle);

    esp_wifi_set_mode(WIFI_MODE_APSTA);
    /* Initialize AP */
    ESP_LOGI(TAG, "INIT AP");
    wifi_init_softap();

    /* Initialize STA */
    ESP_LOGI(TAG, "INIT STA");
    my_wifi_info_t my_wifi_info = {"", ""};
    wifi_init_sta(my_wifi_info);

    esp_wifi_stop();
    ret = esp_wifi_start();
    ESP_LOGI(TAG, "Wifi sta start code: %s", esp_err_to_name(ret));
 
    return ret;
}


esp_err_t wifi_helper_start_sta(my_wifi_info_t my_wifi_info) 
{
    esp_err_t ret = ESP_OK;
    const char * TAG = "WIFI_STA_START";

    ESP_LOGW(TAG, "Unregister return code: %s", esp_err_to_name(ret)); 

    if( _wifi_sta_task_handle == NULL )
    xTaskCreate(wifi_helper_sta_task, "Wifi STA Task", 
                1024 * 6, 
                NULL, 
                1, &_wifi_sta_task_handle);   


    esp_wifi_set_mode(WIFI_MODE_STA);
 
    /* Initialize STA */
    ESP_LOGI(TAG, "INIT STA");
    wifi_init_sta(my_wifi_info);

    esp_wifi_stop();
    ret = esp_wifi_start();
    ESP_LOGI(TAG, "Wifi sta start code: %s", esp_err_to_name(ret));

    return ret;
}

static void wifi_helper_scan_task(void *pv) 
{
    const char * TAG = "WIFI_SCAN_TASK";

    wifi_country_t wifi_country_config = {
        .cc = "CN",
        .schan = 1,
        .nchan = 13,
    };
    ESP_ERROR_CHECK(esp_wifi_set_country(&wifi_country_config));

    while(1) {

        xEventGroupWaitBits(_wifi_scan_event_group,
                            WIFI_SCAN_DONE_BIT,
                            pdFALSE,
                            pdFALSE,
                            portMAX_DELAY);

        ESP_LOGI(TAG, "Wifi Scan Done, ready to print.");
        
        uint16_t ap_num = 0;
        ESP_ERROR_CHECK(esp_wifi_scan_get_ap_num(&ap_num));
        ESP_LOGI(TAG, "AP Count : %d", ap_num);

        uint16_t max_aps = 12;
        wifi_ap_record_t ap_records[max_aps];
        memset(ap_records, 0, sizeof(ap_records));

        uint16_t aps_count = max_aps;
        ESP_ERROR_CHECK(esp_wifi_scan_get_ap_records(&aps_count, ap_records));

        cJSON *root = cJSON_CreateObject();
        cJSON_AddStringToObject(root, "t", "ws");
        cJSON_AddNumberToObject(root, "n", aps_count);
        cJSON *array = cJSON_CreateArray();

        cJSON *entry[aps_count];

        for (int i = 0; i < aps_count; i++){
            entry[i] = cJSON_CreateObject();
            cJSON_AddStringToObject(entry[i], "s", (char *)ap_records[i].ssid);
            cJSON_AddNumberToObject(entry[i], "r", ap_records[i].rssi);
            cJSON_AddNumberToObject(entry[i], "a", ap_records[i].authmode);
            cJSON_AddItemToArray(array, entry[i]);
        }
        cJSON_AddItemToObject(root, "i", array);
        char *payload = cJSON_Print(root);

        // ESP_LOGI(TAG, "The json payload is: %s, strlen is %d", payload, strlen(payload));
        ws_async_send(payload);

        cJSON_Delete(root);

        _scanStarted = 0;
        xEventGroupClearBits(_wifi_scan_event_group, WIFI_SCAN_SCANNING_BIT | WIFI_SCAN_DONE_BIT);
    }
    vTaskDelete(NULL);
    _wifi_scan_task_handle = NULL;
}

static esp_err_t wifi_helper_start_scan() 
{
    esp_err_t ret = ESP_OK;
    const char * TAG = "WIFI_SCAN";

    if (_wifi_scan_task_handle == NULL) {
        ESP_LOGI(TAG, "Wifi scan task not running, starting now");  
        xTaskCreatePinnedToCore(wifi_helper_scan_task, "Wifi Scan Task", 
                                        1024 * 8, 
                                        NULL, 
                                        1, &_wifi_scan_task_handle, 1);    
    }

    EventBits_t bits = xEventGroupGetBits(_wifi_scan_event_group);
    if (bits & WIFI_SCAN_SCANNING_BIT) {
        ESP_LOGI(TAG, "Wifi scan alreday in process.");
        return WIFI_SCAN_RUNNING;
    }
    ESP_LOGI(TAG, "Starting Wifi scan");
    ret = esp_wifi_scan_start(NULL, false);
    ESP_LOGI(TAG, "Wifi scan code: %s", esp_err_to_name(ret));
    if ( ret == ESP_OK) {
        xEventGroupClearBits(_wifi_scan_event_group, WIFI_SCAN_DONE_BIT); 
        xEventGroupSetBits(_wifi_scan_event_group, WIFI_SCAN_SCANNING_BIT); 
    }
    return ret;
}


esp_err_t wifi_helper_init(void) 
{   
    const char * TAG = "WIFI_INIT";
    esp_err_t ret = ESP_OK;

    wifi_init_config_t wifi_init_config = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&wifi_init_config));

    if (_wifi_sta_event_group == NULL) {
        _wifi_sta_event_group = xEventGroupCreate();
    }
    if (_wifi_apsta_event_group == NULL) {
        _wifi_apsta_event_group = xEventGroupCreate();
    }
    if (_wifi_scan_event_group == NULL) {
        _wifi_scan_event_group = xEventGroupCreate();
    }   
    if (_ws_async_msg_queue == NULL) {
        _ws_async_msg_queue = xQueueCreate(2, sizeof(char[800]));
    }       
    if (wifi_connect_lock == NULL) {
        wifi_connect_lock = xSemaphoreCreateMutex();
    }
    ESP_PARAM_CHECK(wifi_connect_lock);

    ESP_ERROR_CHECK(
        esp_event_handler_instance_register(WIFI_EVENT,
            ESP_EVENT_ANY_ID, &wifi_event_handler, NULL, NULL));

    ESP_ERROR_CHECK(
        esp_event_handler_instance_register(IP_EVENT,
            ESP_EVENT_ANY_ID, &ip_event_handler, NULL, NULL));


    my_wifi_info_t* my_wifi_info = (my_wifi_info_t*)calloc(1, sizeof(my_wifi_info_t));

    ret = nvs_helper_blob_get("mywifi", "my_wifi_info", my_wifi_info, sizeof(my_wifi_info_t));
    ESP_ERROR_GOTO(ret != ESP_OK, AP_STA, "Failed to init nvs, starting apsta mode");

    ESP_LOGW(TAG, "Got Wifi info from nvs, stating sta mode");
    wifi_helper_start_sta(*my_wifi_info); 
    free(my_wifi_info);
    return ESP_OK; 

AP_STA:
    wifi_helper_start_apsta();
    free(my_wifi_info);
    return ESP_OK;     
}

static char * _wifi_mode_to_name(wifi_mode_t* mode) 
{
    switch (*mode) {
        case WIFI_MODE_NULL: return "NULL";
        case WIFI_MODE_AP: return "AP";                        
        case WIFI_MODE_STA: return "STA";
        case WIFI_MODE_APSTA: return "APSTA";
        default: return "UNKNOWN";
    }
}

static const char * _wifi_event_to_string(wifi_event_t* event) {
    switch (*event) {
        case WIFI_EVENT_WIFI_READY: return "WIFI_EVENT_WIFI_READY";
        case WIFI_EVENT_SCAN_DONE: return "WIFI_EVENT_SCAN_DONE";
        case WIFI_EVENT_STA_START: return "WIFI_EVENT_STA_START";
        case WIFI_EVENT_STA_STOP: return "WIFI_EVENT_STA_STOP";
        case WIFI_EVENT_STA_CONNECTED: return "WIFI_EVENT_STA_CONNECTED";
        case WIFI_EVENT_STA_DISCONNECTED: return "WIFI_EVENT_STA_DISCONNECTED";
        case WIFI_EVENT_STA_AUTHMODE_CHANGE: return "WIFI_EVENT_STA_AUTHMODE_CHANGE";
        case WIFI_EVENT_STA_WPS_ER_SUCCESS: return "WIFI_EVENT_STA_WPS_ER_SUCCESS";
        case WIFI_EVENT_STA_WPS_ER_FAILED: return "WIFI_EVENT_STA_WPS_ER_FAILED";
        case WIFI_EVENT_STA_WPS_ER_TIMEOUT: return "WIFI_EVENT_STA_WPS_ER_TIMEOUT";
        case WIFI_EVENT_STA_WPS_ER_PIN: return "WIFI_EVENT_STA_WPS_ER_PIN";
        case WIFI_EVENT_STA_WPS_ER_PBC_OVERLAP: return "WIFI_EVENT_STA_WPS_ER_PBC_OVERLAP";
        case WIFI_EVENT_AP_START: return "WIFI_EVENT_AP_START";
        case WIFI_EVENT_AP_STOP: return "WIFI_EVENT_AP_STOP";
        case WIFI_EVENT_AP_STACONNECTED: return "WIFI_EVENT_AP_STACONNECTED";
        case WIFI_EVENT_AP_STADISCONNECTED: return "WIFI_EVENT_AP_STADISCONNECTED";
        case WIFI_EVENT_AP_PROBEREQRECVED: return "WIFI_EVENT_AP_PROBEREQRECVED";
        case WIFI_EVENT_FTM_REPORT: return "WIFI_EVENT_FTM_REPORT";
        case WIFI_EVENT_STA_BSS_RSSI_LOW: return "WIFI_EVENT_STA_BSS_RSSI_LOW";
        case WIFI_EVENT_ACTION_TX_STATUS: return "WIFI_EVENT_ACTION_TX_STATUS";
        case WIFI_EVENT_ROC_DONE: return "WIFI_EVENT_ROC_DONE";
        case WIFI_EVENT_STA_BEACON_TIMEOUT: return "WIFI_EVENT_STA_BEACON_TIMEOUT";
        case WIFI_EVENT_CONNECTIONLESS_MODULE_WAKE_INTERVAL_START: return "WIFI_EVENT_CONNECTIONLESS_MODULE_WAKE_INTERVAL_START";
        case WIFI_EVENT_AP_WPS_RG_SUCCESS: return "WIFI_EVENT_AP_WPS_RG_SUCCESS";
        case WIFI_EVENT_AP_WPS_RG_FAILED: return "WIFI_EVENT_AP_WPS_RG_FAILED";
        case WIFI_EVENT_AP_WPS_RG_TIMEOUT: return "WIFI_EVENT_AP_WPS_RG_TIMEOUT";
        case WIFI_EVENT_AP_WPS_RG_PIN: return "WIFI_EVENT_AP_WPS_RG_PIN";
        case WIFI_EVENT_AP_WPS_RG_PBC_OVERLAP: return "WIFI_EVENT_AP_WPS_RG_PBC_OVERLAP";
        case WIFI_EVENT_ITWT_SETUP: return "WIFI_EVENT_ITWT_SETUP";
        case WIFI_EVENT_ITWT_TEARDOWN: return "WIFI_EVENT_ITWT_TEARDOWN";
        case WIFI_EVENT_ITWT_PROBE: return "WIFI_EVENT_ITWT_PROBE";
        case WIFI_EVENT_ITWT_SUSPEND: return "WIFI_EVENT_ITWT_SUSPEND";
        case WIFI_EVENT_NAN_STARTED: return "WIFI_EVENT_NAN_STARTED";
        case WIFI_EVENT_NAN_STOPPED: return "WIFI_EVENT_NAN_STOPPED";
        case WIFI_EVENT_NAN_SVC_MATCH: return "WIFI_EVENT_NAN_SVC_MATCH";
        case WIFI_EVENT_NAN_REPLIED: return "WIFI_EVENT_NAN_REPLIED";
        case WIFI_EVENT_NAN_RECEIVE: return "WIFI_EVENT_NAN_RECEIVE";
        case WIFI_EVENT_NDP_INDICATION: return "WIFI_EVENT_NDP_INDICATION";
        case WIFI_EVENT_NDP_CONFIRM: return "WIFI_EVENT_NDP_CONFIRM";
        case WIFI_EVENT_NDP_TERMINATED: return "WIFI_EVENT_NDP_TERMINATED";
        case WIFI_EVENT_HOME_CHANNEL_CHANGE: return "WIFI_EVENT_HOME_CHANNEL_CHANGE";
        default: return "UNKNOWN";
    }
}