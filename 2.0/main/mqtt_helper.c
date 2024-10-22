#include "mqtt_helper.h"

static esp_mqtt_client_handle_t CLIENT;

static bool _is_connected = false;

// const esp_mqtt_topic_t recv_cmd_topic = {"recv_cmd", 1};
// const esp_mqtt_topic_t send_cmd_topic = {"send_cmd", 1};
// const esp_mqtt_topic_t sync_cmd_topic = {"sync_cmd", 1};

const esp_mqtt_topic_t topics[] = {
    {MQTT_TOPIC_RECV_CMD, 1},
    {MQTT_TOPIC_SEND_CMD, 1},
    {MQTT_TOPIC_SYNC_CMD, 1}
};

static void log_error_if_nonzero(const char *message, int error_code)
{
    if (error_code != 0) {
        ESP_LOGE("MQTT ERROR", "Last error %s: 0x%x", message, error_code);
    }
}

int8_t mqtt_enqueue_from_list(const char* base_path, const esp_mqtt_topic_t* topic_list, size_t len, char* msg, int qos)
{
    const char * TAG = "MQTT ENQUELIST"; 
    const size_t base_pathlen = strlen(base_path);
    char* full_topic = malloc(sizeof(char[MAX_TOPIC_PATH_LENGTH + 1]));

    for(int i = 0; i < len; i++) {
        size_t offset1 = 0;
        size_t offset2 = 0;
        if (base_path[0] != '/')            offset1 = 1;
        if (topic_list[i].filter[0] != '/') offset2 = 1;        
        if ((strlen(topic_list[i].filter) + strlen(base_path) + offset1 + offset2) 
                                                        > MAX_TOPIC_PATH_LENGTH) {
            free(full_topic);
            return -2;
        }

        if (base_path[0] != '/')            full_topic[0] = '/';
        strcpy(full_topic + offset1, base_path);

        if (topic_list[i].filter[0] != '/') full_topic[offset1 + base_pathlen] = '/';
        strcpy(full_topic + offset1 + offset2 + base_pathlen, topic_list[i].filter);

        int msg_id = esp_mqtt_client_enqueue(CLIENT, full_topic, msg, 0, qos, true, true);
        if (msg_id == 0) ESP_LOGI(TAG, "Enqueue for %s successful", full_topic);  

        memset(full_topic, 0, sizeof(char[MAX_TOPIC_LENGTH]));
    }
    free(full_topic);

    return 0;
}

int8_t mqtt_subscribe_from_list(const char* base_path, const esp_mqtt_topic_t* topic_list, size_t len)
{
    const char * TAG = "MQTT SUBLIST"; 
    const size_t base_pathlen = strlen(base_path);
    char* full_topic = malloc(sizeof(char[MAX_TOPIC_PATH_LENGTH + 1]));

    for(int i = 0; i < len; i++) {
        size_t offset1 = 0;
        size_t offset2 = 0;
        if (base_path[0] != '/') offset1 = 1;
        if (topic_list[i].filter[0] != '/') offset2 = 1;        
        if ((strlen(topic_list[i].filter) + strlen(base_path) + offset1 + offset2) > MAX_TOPIC_PATH_LENGTH) {
            free(full_topic);
            return -2;
        }

        if (base_path[0] != '/') full_topic[0] = '/';
        strcpy(full_topic + offset1, base_path);

        if (topic_list[i].filter[0] != '/') full_topic[offset1 + base_pathlen] = '/';
        strcpy(full_topic + offset1 + offset2 + base_pathlen, topic_list[i].filter);
 
        int msg_id = esp_mqtt_client_subscribe(CLIENT, full_topic, 1);
        if (msg_id == 0) ESP_LOGI(TAG, "Subscribe for %s successful", full_topic);        
        memset(full_topic, 0, sizeof(char[MAX_TOPIC_LENGTH]));
    }
    free(full_topic);

    return 0;
}

int8_t mqtt_unsubscribe_from_list(const char* base_path, const esp_mqtt_topic_t* topic_list, size_t len)
{
    const char * TAG = "MQTT UNSUBLIST"; 
    const size_t base_pathlen = strlen(base_path);
    char* full_topic = malloc(sizeof(char[MAX_TOPIC_PATH_LENGTH + 1]));

    for(int i = 0; i < len; i++) {
        size_t offset1 = 0;
        size_t offset2 = 0;
        if (base_path[0] != '/') offset1 = 1;
        if (topic_list[i].filter[0] != '/') offset2 = 1;        
        if ((strlen(topic_list[i].filter) + strlen(base_path) + offset1 + offset2) > MAX_TOPIC_PATH_LENGTH) {
            free(full_topic);
            return -2;
        }

        if (base_path[0] != '/') full_topic[0] = '/';
        strcpy(full_topic + offset1, base_path);

        if (topic_list[i].filter[0] != '/') full_topic[offset1 + base_pathlen] = '/';
        strcpy(full_topic + offset1 + offset2 + base_pathlen, topic_list[i].filter);

        int msg_id = esp_mqtt_client_unsubscribe(CLIENT, full_topic);
        if (msg_id == 0) ESP_LOGI(TAG, "Unsubscribe for %s successful", full_topic);        
        memset(full_topic, 0, sizeof(char[MAX_TOPIC_LENGTH]));
    }
    free(full_topic);

    return 0;
}

static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    const char * TAG = "MQTT HANDLER";

    // ESP_LOGI(TAG, "Event base=%s, event_id=%" PRIi32, base, event_id);
    esp_mqtt_event_handle_t event = event_data;

    int msg_id;

    switch ((esp_mqtt_event_id_t)event_id) {
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
        _is_connected = true;
        break;
    case MQTT_EVENT_DISCONNECTED:
        _is_connected = false;
        ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
        break;
    case MQTT_EVENT_SUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_UNSUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_PUBLISHED:
        ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_DATA:
        ESP_LOGI(TAG, "MQTT_EVENT_DATA");
        printf("TOPIC: %.*s | DATA: %.*s\r\n", event->topic_len, event->topic, event->data_len, event->data);
        break;
    case MQTT_EVENT_ERROR:
        ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
        if (event->error_handle->error_type == MQTT_ERROR_TYPE_TCP_TRANSPORT) {
            log_error_if_nonzero("reported from esp-tls", event->error_handle->esp_tls_last_esp_err);
            log_error_if_nonzero("reported from tls stack", event->error_handle->esp_tls_stack_err);
            log_error_if_nonzero("captured as transport's socket errno",  event->error_handle->esp_transport_sock_errno);
            ESP_LOGI(TAG, "Last errno string (%s)", strerror(event->error_handle->esp_transport_sock_errno));
        }
        break;
    default:
        ESP_LOGI(TAG, "Other event id:%d", event->event_id);
        break;
    }
}

esp_err_t mqtt_helper_start(esp_mqtt_client_config_t mqtt_cfg)
{   
    const char * TAG = "MQTT START"; 
    esp_err_t ret = ESP_OK;

    CLIENT = esp_mqtt_client_init(&mqtt_cfg);
    if (CLIENT == NULL) {
        ESP_LOGE(TAG, "MQTT CLIENT Init failed.");
        return ESP_FAIL;
    }

    esp_mqtt_client_register_event(CLIENT, 
                                    ESP_EVENT_ANY_ID, 
                                    mqtt_event_handler, 
                                    NULL);

    ret = esp_mqtt_client_start(CLIENT);

    if (ret == ESP_OK) ESP_LOGI(TAG, "MQTT CLIENT started");
    else ESP_LOGW(TAG, "MQTT CLIENT start failed");
    return ret;
}

esp_err_t mqtt_helper_stop()
{   
    const char * TAG = "MQTT STOP"; 
    esp_err_t ret = ESP_OK;

    if (CLIENT == NULL) return ret;

    ESP_LOG_INFO(esp_mqtt_client_unregister_event(CLIENT, 
                                    ESP_EVENT_ANY_ID, 
                                    mqtt_event_handler),
                "unregister event failed");

    ret = esp_mqtt_client_disconnect(CLIENT);
    ESP_ERROR_RETURN(ret != ESP_OK, ret, "Disconnect failed");

    ret = esp_mqtt_client_stop(CLIENT);
    ESP_ERROR_RETURN(ret != ESP_OK, ret, "Stop failed");

    ret = esp_mqtt_client_destroy(CLIENT);
    ESP_ERROR_RETURN(ret != ESP_OK, ret, "Destroy failed");

    return ret;
}

bool mqtt_helper_is_connected()
{
    return _is_connected;
}

