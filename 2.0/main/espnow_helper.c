#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/param.h>

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"

#include "esp_wifi.h"
#include "esp_sleep.h"
#include "esp_now.h"
#include "esp_log.h"
#include "esp_crc.h"
#include "esp_mac.h"
#include "esp_random.h"

#include "espnow_helper.h"

#include "my_rtos.h"

#define SEND_CB_OK                      BIT0
#define SEND_CB_FAIL                    BIT1
#define ESPNOW_MSG_CACHE                32

static EventGroupHandle_t g_event_group  = NULL;
static QueueHandle_t      g_espnow_queue = NULL;
static QueueHandle_t      g_ack_queue    = NULL;
static SemaphoreHandle_t  g_send_lock    = NULL;

static struct {
    uint8_t type;
    uint16_t magic;
}   __attribute__((packed)) \
    g_msg_magic_cache[ESPNOW_MSG_CACHE] = {0};
static uint8_t g_msg_magic_cache_next = 0;

static espnow_addr_t ESPNOW_ADDR_SELF              = {0};
static const espnow_addr_t ESPNOW_ADDR_BROADCAST   = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0XFF};

static espnow_addr_t g_master_addr = {0};
static uint8_t g_master_channel = 0;
static espnow_config_t *g_espnow_config = NULL;
static espnow_frame_head_t g_espnow_frame_head_default = ESPNOW_FRAME_CONFIG_DEFAULT();

static const char *TAG = "ESP-NOW";

/**
 * @brief Translate espnow_data_type_t to string
 * 
 * @param type The espnow_data_type_t to translate
 * @return const char* The string representation of the type
 */
static const char* espnow_data_type_to_string(espnow_data_type_t type) {
    switch (type) {
        case ESPNOW_DATA_TYPE_ACK:
            return "ACK";
        case ESPNOW_DATA_TYPE_FORWARD:
            return "FORWARD";
        case ESPNOW_DATA_TYPE_GROUP:
            return "GROUP";
        case ESPNOW_DATA_TYPE_CONTROL:
            return "CONTROL";
        case ESPNOW_DATA_TYPE_DEBUG:
            return "DEBUG";
        case ESPNOW_DATA_TYPE_DATA:
            return "DATA";
        default:
            return "UNKNOWN";
    }
}

static void espnow_send_task(void *pvParameter)
{
    const char *TAG = "ESP-NOW-SEND_TASK";
    // espnow_addr_t dest_addr = {0}; // Example destination MAC address
    char data[20] = {0};
    static uint8_t num = 0;
    uint32_t counter = 0; // Initialize counter to send
    esp_err_t err;

    while (1) {
        memset(data , 0, sizeof(data));
        snprintf(data, sizeof(data), "This is %d", num++);
        printf("----------------\n"); fflush(stdout);
        ESP_LOGI(TAG, "Sending Msg: [%s] to "MACSTR"", data, MAC2STR(g_master_addr));
        err = espnow_send(ESPNOW_DATA_TYPE_CONTROL, g_master_addr, data, sizeof(data), true);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Error sending ESPNOW data: %s", esp_err_to_name(err));
        }
        counter++; // Increment the counter for the next send
        vTaskDelay(pdMS_TO_TICKS(5000)); // Wait for 1 second
    }
}

void start_espnow_send_task()
{
    xTaskCreate(espnow_send_task, "espnow_send_task", 2048, NULL, 1, NULL);
}


esp_err_t espnow_add_peer(const espnow_addr_t addr, uint8_t channel)
{
    ESP_PARAM_CHECK(addr);

    /**< If peer exists, delete a peer from peer list */
    if (esp_now_is_peer_exist(addr)) {
        return ESP_OK;
    }

    esp_err_t ret = ESP_OK;
    esp_now_peer_info_t peer = {
        .ifidx = WIFI_IF_STA,
        .channel = channel,
    };

    memcpy(peer.peer_addr, addr, 6);

    /**< Add a peer to peer list */
    ret = esp_now_add_peer(&peer);
    ESP_ERROR_RETURN(ret != ESP_OK, ret, "Add a peer to peer list fail");

    return ESP_OK;
}

esp_err_t espnow_mod_peer(const espnow_addr_t addr, const esp_now_peer_info_t *peer_info)
{
    ESP_PARAM_CHECK(addr);
    ESP_PARAM_CHECK(peer_info);

    /**< If peer does not exist, return error */
    if (!esp_now_is_peer_exist(addr)) {
        return ESP_ERR_ESPNOW_NOT_FOUND;
    }

    esp_err_t ret = esp_now_mod_peer(peer_info);
    ESP_ERROR_RETURN(ret != ESP_OK, ret, "Modify a peer in peer list fail");

    return ESP_OK;
}

esp_err_t espnow_del_peer(const espnow_addr_t addr)
{
    ESP_PARAM_CHECK(addr);

    esp_err_t ret = ESP_OK;

    /**< If peer exists, delete a peer from peer list */
    if (esp_now_is_peer_exist(addr) && !ESPNOW_ADDR_IS_BROADCAST(addr)) {
        ret = esp_now_del_peer(addr);
        ESP_ERROR_RETURN(ret != ESP_OK, ret, "esp_now_del_peer fail, ret: %d", ret);
    }

    return ESP_OK;
}

static bool queue_over_write(espnow_msg_type_t msg_type, const void *const data, size_t data_len, void *arg, TickType_t xTicksToWait)
{
    if (msg_type == ESPNOW_MSG_TYPE_RECV_ACK) {
        ESP_PARAM_CHECK(g_ack_queue);
        return xQueueSend(g_ack_queue, &data, xTicksToWait);

    } else {
        ESP_PARAM_CHECK(g_espnow_queue);

        espnow_event_ctx_t espnow_event = {
            .msg_type = msg_type,
            .data_len = data_len,
            .data = (void *)data,
            .handle = arg
        };

        return xQueueSend(g_espnow_queue, &espnow_event, xTicksToWait);
    }
}


void espnow_recv_cb(const esp_now_recv_info_t *recv_info, const uint8_t *data, int size)
{
    // Cast received data to espnow_data_t for easier handling
    const char *TAG = "ESP_NOW_RECV_CB";
    espnow_data_t *espnow_data = (espnow_data_t *)data;

    // Extract source address and RX control information
    uint8_t *addr = recv_info->src_addr;
    wifi_pkt_rx_ctrl_t *rx_ctrl = recv_info->rx_ctrl;
    espnow_frame_head_t *frame_head = &espnow_data->frame_head;

    for (size_t i = 0, index = g_msg_magic_cache_next; i < ESPNOW_MSG_CACHE;
            i++, index = (g_msg_magic_cache_next + i) % ESPNOW_MSG_CACHE) {
        if (g_msg_magic_cache[index].type == espnow_data->type
                && g_msg_magic_cache[index].magic == frame_head->magic) {
            ESP_LOGI(TAG, "Duplicate message: 0x%x, discard", frame_head->magic);
            return ;
        }
    }
    printf("----------------\n"); fflush(stdout);
    ESP_LOGI(TAG, "Dst:["MACSTR"] | type: %s "
                  "| Data: [%s] " 
                  "| Rx: %d | size: %d | rssi: %d | magic: 0x%x | ack: %d",
             MAC2STR(espnow_data->dest_addr), espnow_data_type_to_string(espnow_data->type), (char *)espnow_data->data, 
             rx_ctrl->channel, espnow_data->size, rx_ctrl->rssi,
             frame_head->magic, espnow_data->frame_head.ack);

    // Validate the received data
    if ((espnow_data->type >= ESPNOW_DATA_TYPE_MAX) || (size != espnow_data->size + sizeof(espnow_data_t))
        || ESPNOW_ADDR_IS_SELF(espnow_data->src_addr)) {
        ESP_LOGI(TAG, "Invalid receive callback arguments, recv_addr: "MACSTR", src_addr: " MACSTR ", data: %p, size: %d",
                 MAC2STR(addr), MAC2STR(espnow_data->src_addr), data, size);
        return;
    }

    /**< Channel filtering */
    if (frame_head->channel != rx_ctrl->channel) {
        ESP_LOGI(TAG, "Filter adjacent channels, Channels: [Fr:%d] [Rx:%d]", frame_head->channel, rx_ctrl->channel);
        return ;
    }

    /**< Rssi filtering */
    if (-80 > rx_ctrl->rssi) {
        ESP_LOGI(TAG, "Filter weak signal strength, RSSI: %d < -80dB",  rx_ctrl->rssi);
        return ;
    }

    if (espnow_data->type != ESPNOW_DATA_TYPE_ACK && espnow_data->type != ESPNOW_DATA_TYPE_GROUP
        && frame_head->ack && ESPNOW_ADDR_IS_SELF(espnow_data->dest_addr)) {
        espnow_data_t *ack_data = ESP_CALLOC(1, sizeof(espnow_data_t));

        ack_data->type  = ESPNOW_DATA_TYPE_ACK;
        ack_data->size  = 0;
        memcpy(&ack_data->frame_head, frame_head, sizeof(espnow_frame_head_t));
        memcpy(ack_data->src_addr, ESPNOW_ADDR_SELF, 6);
        memcpy(ack_data->dest_addr, espnow_data->src_addr, 6);

        ack_data->frame_head.forward = true;
        ack_data->frame_head.ack = false;

        if (!g_espnow_queue || queue_over_write(ESPNOW_MSG_TYPE_SEND_ACK, ack_data, sizeof(espnow_data_t), NULL, g_espnow_config->send_max_timeout) != pdPASS) {
            ESP_LOGW(TAG, "[%s, %d] Send event queue failed", __func__, __LINE__);
            ESP_FREE(ack_data);
        }
    }

    if (espnow_data->type == ESPNOW_DATA_TYPE_ACK) {
        ESP_LOGI(TAG, "Received ACK");
        if (!ESPNOW_ADDR_IS_SELF(espnow_data->dest_addr)) {
            goto FORWARD_DATA;
        }

        uint32_t *magic = ESP_MALLOC(sizeof(uint32_t));
        *magic = frame_head->magic;

        if (!g_ack_queue || queue_over_write(ESPNOW_MSG_TYPE_RECV_ACK, magic, sizeof(uint32_t), NULL, g_espnow_config->send_max_timeout) != pdPASS) {
            ESP_LOGW(TAG, "[%s, %d] Send event queue failed", __func__, __LINE__);
            ESP_FREE(magic);
            return ;
        };

        goto EXIT;

    } else if (espnow_data->type == ESPNOW_DATA_TYPE_GROUP) {
        ESP_LOGI(TAG, "Received GROUP");
        espnow_group_info_t *group_info = (espnow_group_info_t *) espnow_data->data;
        bool set_group_flag = false;

        ESP_LOGI(TAG, ">1.2< group_id: " MACSTR ", dest_addr: " MACSTR,
                 MAC2STR(group_info->group_id), MAC2STR(espnow_data->dest_addr));
        ESP_LOGI(TAG, ">1.2< addr_mun: %d, dest_addr: " MACSTR, group_info->addrs_num,
                 MAC2STR(group_info->addrs_list[0]));

        if (group_info->addrs_num == 1 && ESPNOW_ADDR_IS_BROADCAST(group_info->addrs_list[0])) {
            set_group_flag = true;
        } else {
            for (size_t i = 0; i < group_info->addrs_num; i++) {
                if (ESPNOW_ADDR_IS_SELF(group_info->addrs_list[i])) {
                    set_group_flag = true;
                    break;
                }
            }
        }

        if (set_group_flag) {
            g_master_channel = group_info->master_channel;
            esp_err_t ret = esp_wifi_set_channel(group_info->master_channel, WIFI_SECOND_CHAN_NONE);
            if (ret == ESP_OK) {
                ESP_LOGI(TAG, "Set channel to master, channel: %d", group_info->master_channel);
            } else {
                ESP_LOGW(TAG, "Set channel to master failed, ret: %d", ret);
                return;
            }
            esp_now_peer_info_t all = {
                .ifidx = WIFI_IF_STA,
                .channel = group_info->master_channel,
            };            
            espnow_mod_peer(ESPNOW_ADDR_BROADCAST, &all);

            if (group_info->join) {
                if (!esp_now_is_peer_exist(group_info->master_id)) {
                    espnow_add_peer(group_info->master_id, group_info->master_channel);
                    memcpy(g_master_addr, group_info->master_id, ESPNOW_ADDR_LEN);
                    ESP_LOGI(TAG, "Add the master peer, " MACSTR, MAC2STR(g_master_addr));
                    start_espnow_send_task();
                    ESP_LOGI(TAG, "Start the espnow send task");
                }
                else {
                    ESP_LOGI(TAG, "The master peer already exist");
                }
            } else {
                espnow_del_peer(group_info->master_id);
                memset(g_master_addr, 0, ESPNOW_ADDR_LEN);
                ESP_LOGI(TAG, "Delete the master peer, " MACSTR, MAC2STR(g_master_addr));
            }
        }

         goto EXIT;

    } else {
        ESP_LOGI(TAG, "Received DATA");
        if (!frame_head->group && frame_head->forward && !ESPNOW_ADDR_IS_BROADCAST(espnow_data->dest_addr)
                && !ESPNOW_ADDR_IS_SELF(espnow_data->dest_addr)) {
            goto FORWARD_DATA;
        } 
        // else if (frame_head->group && !espnow_is_my_group(espnow_data->dest_addr)) {
        //     ESP_LOGI(TAG, "[%s, %d]: group_num: %d, group_id: " MACSTR,
        //              __func__, __LINE__, espnow_get_group_num(), MAC2STR(espnow_data->dest_addr));
        //     goto FORWARD_DATA;
        // }
        espnow_data_t *q_data = ESP_MALLOC(size);

        memcpy(q_data, espnow_data, size);

        if (queue_over_write(ESPNOW_MSG_TYPE_RECEIVE, q_data, size, NULL, g_espnow_config->send_max_timeout) != pdPASS) {
            ESP_LOGW(TAG, "[%s, %d] Send event queue failed", __func__, __LINE__);
            ESP_FREE(q_data);
            return ;
        }

        goto EXIT;
    }

    goto EXIT;

FORWARD_DATA:

    /**< Data does not need to be forwarded */
    if ((!g_espnow_config->forward_enable || !frame_head->forward || !frame_head->forward_ttl)) {
        ESP_LOGI(TAG, "Data does not need to be forwarded");
        return;
    }


    ESP_LOGI(TAG, "Forwarding data");
    if (g_espnow_config->forward_enable && frame_head->forward_ttl > 0 && frame_head->forward
            && !ESPNOW_ADDR_IS_SELF(espnow_data->dest_addr)
            && !ESPNOW_ADDR_IS_SELF(espnow_data->src_addr)) {
        espnow_data_t *q_data = ESP_MALLOC(size);

        memcpy(q_data, espnow_data, size);

        if (frame_head->forward_ttl > 0) {
            q_data->frame_head.forward_ttl--;
        }

        if (!g_espnow_queue || queue_over_write(ESPNOW_MSG_TYPE_FORWARD, q_data, size, NULL, g_espnow_config->send_max_timeout) != pdPASS) {
            ESP_LOGW(TAG, "[%s, %d] Send event queue failed", __func__, __LINE__);
            ESP_FREE(q_data);
            return ;
        }
    }

EXIT:
    ESP_LOGI(TAG, "No error, Exiting");
    g_msg_magic_cache_next = (g_msg_magic_cache_next + 1) % ESPNOW_MSG_CACHE;
    g_msg_magic_cache[g_msg_magic_cache_next].type  = espnow_data->type;
    g_msg_magic_cache[g_msg_magic_cache_next].magic = frame_head->magic;
}


/**< callback function of sending ESPNOW data */
void espnow_send_cb(const uint8_t *addr, esp_now_send_status_t status)
{

    if (!addr || !g_event_group) {
        ESP_LOGW(TAG, "Send cb args error, addr is NULL");
        return ;
    }

    if (status == ESP_NOW_SEND_SUCCESS) {
        xEventGroupSetBits(g_event_group, SEND_CB_OK);
    } else {
        xEventGroupSetBits(g_event_group, SEND_CB_FAIL);
    }
}



static esp_err_t espnow_send_process(espnow_data_t *espnow_data, bool *ack, int retry_count, uint32_t wait_ticks)
{
    const char* TAG = "ESP_NOW_SEND_PROCESS";
    ESP_PARAM_CHECK(espnow_data);
    ESP_PARAM_CHECK(g_ack_queue);

    espnow_frame_head_t *frame_head = &espnow_data->frame_head;
    bool is_broadcast = frame_head->forward;
    bool requires_ack = frame_head->ack;

    ESP_LOGI(TAG, "is_broadcast: %d, requires_ack: %d", is_broadcast, requires_ack);
    
    // Handle forward or unicast packets that require acknowledgment
    if (!requires_ack && ack) {
        // ESP_LOGI(TAG, "Waiting for send callback event bits");
        EventBits_t uxBits = xEventGroupWaitBits(g_event_group, SEND_CB_OK | SEND_CB_FAIL,
                                pdTRUE, pdFALSE,  MIN(wait_ticks, g_espnow_config->send_max_timeout));
        if (uxBits & SEND_CB_OK) {
            *ack = true;
            ESP_LOGI(TAG, "ACK set to true for non-forward and no ack required");
            return ESP_OK;
        } else {
            ESP_LOGE(TAG, "Failed to receive SEND_CB_OK event bit");
            return ESP_FAIL;
        }
    }

    // Handle acknowledgment for unicast packets
    // 需要ack, 需要转发, 但收件人是个体
    if (requires_ack && !ESPNOW_ADDR_IS_BROADCAST(espnow_data->dest_addr) && ack) {
        // ESP_LOGI(TAG, "Waiting for ack reply");
        uint32_t delay_ms = (retry_count < 6 ? 1 << retry_count : 50) * 30; // Calculate backoff time
        // ESP_LOGI(TAG, "Calculated backoff time: %ld ms | wait_ticks: %ld ms", delay_ms, wait_ticks);

        uint32_t *ack_magic = NULL;
        if (xQueueReceive(g_ack_queue, &ack_magic, delay_ms) == pdPASS && *ack_magic == frame_head->magic) {
            ESP_LOGI(TAG, "ACK received with matching magic");
            espnow_data->frame_head.ack = false;
            ESP_FREE(ack_magic);
            *ack = true;
            return ESP_OK;
        }
        ESP_FREE(ack_magic);

        ESP_LOGE(TAG, "Timeout waiting for ACK");
        return ESP_ERR_WIFI_TIMEOUT;
    }

    ESP_LOGI(TAG, "Didn't process, return fail");
    return ESP_FAIL;
}

esp_err_t espnow_send(espnow_data_type_t type, const espnow_addr_t dest_addr, const void *data,
                      size_t size, bool forward)
{
    ESP_PARAM_CHECK(dest_addr);
    ESP_PARAM_CHECK(data);
    ESP_PARAM_CHECK(type < ESPNOW_DATA_TYPE_MAX);
    ESP_PARAM_CHECK(size <= ESPNOW_MAX_DATA_LEN);
    ESP_ERROR_RETURN(!g_espnow_config, ESP_ERR_ESPNOW_NOT_INIT, "ESPNOW is not initialized");

    const char* TAG = "ESP_NOW_SEND";
    esp_err_t ret             = ESP_OK;
    TickType_t wait_ticks     = g_espnow_config->send_max_timeout;
    uint32_t start_ticks      = xTaskGetTickCount();

    uint8_t primary           = 0;
    wifi_second_chan_t second = 0;
    wifi_country_t self_country;
    ESP_ERR_FUNC_RET(esp_wifi_get_country(&self_country), "Failed to get country");
    ESP_ERR_FUNC_RET(esp_wifi_get_channel(&primary, &second), "esp_wifi_get_channel, err_name: %s", esp_err_to_name(ret));


    espnow_data_t *espnow_data = ESP_MALLOC(sizeof(espnow_data_t) + size);
    ESP_ERROR_RETURN(!espnow_data, ESP_ERR_NO_MEM, "Not enough memory!");
    espnow_data->size = size;
    espnow_data->type = type;
    memcpy(espnow_data->dest_addr, dest_addr, sizeof(espnow_data->dest_addr));
    memcpy(espnow_data->src_addr, ESPNOW_ADDR_SELF, sizeof(espnow_data->src_addr));    
    memcpy(espnow_data->data, data, size);


    espnow_frame_head_t *frame_head = &espnow_data->frame_head;
    memcpy(frame_head, &g_espnow_frame_head_default, sizeof(espnow_frame_head_t));
    frame_head->magic = esp_random();
    if (forward) {
        frame_head->forward = true;
    } else {
        frame_head->forward = false;
    }
    frame_head->channel = primary;
    frame_head->ack       = true;

    ESP_LOGI(TAG, ">>>>Dst: ["MACSTR"] | Type: %s | Data: [%s] ", MAC2STR(dest_addr),
               espnow_data_type_to_string(espnow_data->type), espnow_data->data);

    /**< Wait for other tasks to be sent before send ESP-NOW data */
    if (xSemaphoreTake(g_send_lock, pdMS_TO_TICKS(wait_ticks)) != pdPASS) {
        ESP_FREE(espnow_data);
        return ESP_ERR_TIMEOUT;
    }

    bool send_success = false;

    for (int count = 0; count < g_espnow_config->send_retry_num && !send_success; ++count) {

        if (forward) {
            ret = esp_now_send(ESPNOW_ADDR_BROADCAST, (uint8_t *)espnow_data, espnow_data->size + sizeof(espnow_data_t));
        } else {
            ret = esp_now_send(dest_addr, (uint8_t *)espnow_data, espnow_data->size + sizeof(espnow_data_t));
        }
        if (ret == ESP_OK) {
            bool ack = false;
            TickType_t write_ticks = (wait_ticks == portMAX_DELAY) ? portMAX_DELAY :
                        xTaskGetTickCount() - start_ticks < wait_ticks ?
                        wait_ticks - (xTaskGetTickCount() - start_ticks) : 0;
            ret = espnow_send_process(espnow_data, &ack, count, write_ticks);
            if (ret == ESP_OK && ack) {
                send_success = true;
                break; // Exit the loop if send was successful and acknowledged
            } else {
                ESP_LOGW(TAG, "Sent ackknowledge failed");
            }
        } else {
            ESP_LOGE(TAG, "Sent failed: %s channel: %d retry: %d", esp_err_to_name(ret), 
                                                        primary, count);
        }
    }

    if (!send_success) {
        ESP_LOGE(TAG, "Failed to send ESP-NOW data after %d retries.", g_espnow_config->send_retry_num);
        ret = ESP_FAIL; 
    } else {
        ESP_LOGI(TAG, "Send ESP-NOW data successfully");
    }
    

    xSemaphoreGive(g_send_lock);
    ESP_FREE(espnow_data);
    return ret;
}


esp_err_t espnow_set_group(const uint8_t addrs_list[][ESPNOW_ADDR_LEN], size_t addrs_num,
                           const uint8_t group_id[ESPNOW_ADDR_LEN], bool join)
{
    const char* TAG           = "ESP_NOW_SET_GROUP";
    esp_err_t ret             = ESP_OK;
    uint32_t start_ticks      = xTaskGetTickCount();
    TickType_t wait_ticks     = g_espnow_config->send_max_timeout;

    uint8_t primary           = 0;
    wifi_second_chan_t second = 0;
    wifi_country_t self_country;
    ESP_ERR_FUNC_RET(esp_wifi_get_country(&self_country), "Failed to get country");
    ESP_ERR_FUNC_RET(esp_wifi_get_channel(&primary, &second), "esp_wifi_get_channel, err_name: %s", esp_err_to_name(ret));


    espnow_data_t *espnow_data = ESP_MALLOC(sizeof(espnow_data_t) + sizeof(espnow_group_info_t)     
                                            + addrs_num * ESPNOW_ADDR_LEN);
    ESP_ERROR_RETURN(!espnow_data, ESP_ERR_NO_MEM, "Not enough memory!");    
    espnow_data->size = sizeof(espnow_group_info_t) + addrs_num * ESPNOW_ADDR_LEN;                                    
    espnow_data->type = ESPNOW_DATA_TYPE_GROUP;
    memcpy(espnow_data->dest_addr, ESPNOW_ADDR_BROADCAST, ESPNOW_ADDR_LEN);
    memcpy(espnow_data->src_addr,  ESPNOW_ADDR_SELF, ESPNOW_ADDR_LEN);

    espnow_frame_head_t *frame_head = &espnow_data->frame_head;
    memcpy(frame_head, &g_espnow_frame_head_default, sizeof(espnow_frame_head_t));
    frame_head->magic     = esp_random();
    frame_head->forward = true;
    frame_head->channel   = primary;
    frame_head->ack       = false;

    espnow_group_info_t *group_info = (espnow_group_info_t *) espnow_data->data;
    group_info->join = join;
    group_info->addrs_num = addrs_num;
    group_info->master_channel = primary;
    memcpy(group_info->master_id, ESPNOW_ADDR_SELF, ESPNOW_ADDR_LEN);
    memcpy(group_info->group_id, group_id, ESPNOW_ADDR_LEN);
    memcpy(group_info->addrs_list, addrs_list, addrs_num * ESPNOW_ADDR_LEN);

    ESP_LOGI(TAG, "[%s, %d] addr: " MACSTR", size: %d, data: %s, magic: 0x%x",
             __func__, __LINE__, MAC2STR(ESPNOW_ADDR_BROADCAST), espnow_data->size,
              espnow_data->data, frame_head->magic);

    /**< Wait for other tasks to be sent before send ESP-NOW data */
    if (xSemaphoreTake(g_send_lock, pdMS_TO_TICKS(wait_ticks)) != pdPASS) {
        ESP_FREE(espnow_data);
        return ESP_ERR_TIMEOUT;
    }

    /* Need to disconnect before changing channel*/
    if (xSemaphoreTake(wifi_connect_lock, 0) != pdPASS) {
        xSemaphoreGive(g_send_lock);
        ESP_FREE(espnow_data);
        return ESP_ERR_NOT_ALLOWED;
    }
    ESP_ERR_FUNC_RET(esp_wifi_disconnect(), "Disconnect wifi failed");

    int max_channels = self_country.nchan;
    for (int i = 0; i < max_channels; ++i) {

        bool send_success = false;
        esp_wifi_set_channel(self_country.schan + i, WIFI_SECOND_CHAN_NONE);
        frame_head->channel   = self_country.schan + i;


        for (int count = 0; count < g_espnow_config->send_retry_num && !send_success; ++count) {

            ret = esp_now_send(ESPNOW_ADDR_BROADCAST, (uint8_t *)espnow_data, espnow_data->size + sizeof(espnow_data_t));

            if (ret == ESP_OK) {
                bool ack = false;
                TickType_t write_ticks = (wait_ticks == portMAX_DELAY) ? portMAX_DELAY :
                            xTaskGetTickCount() - start_ticks < wait_ticks ?
                            wait_ticks - (xTaskGetTickCount() - start_ticks) : 0;
                ret = espnow_send_process(espnow_data, &ack, count, write_ticks);
                if (ret == ESP_OK && ack) {
                    send_success = true;
                    break; // Exit the loop if send was successful and acknowledged
                } else {
                    ESP_LOGW(TAG, "Sent ackknowledge failed");
                }
            } else {
                ESP_LOGE(TAG, "Sent failed: %s channel: %d retry: %d", esp_err_to_name(ret), 
                                                            primary, count);
            }
        }

        uint8_t now = 0; wifi_second_chan_t now2 = 0;
        esp_wifi_get_channel(&now, &now2);
        if (!send_success) {
            ESP_LOGE(TAG, "Failed to send ESP-NOW data after %d retries, channel: %d", 
                            i, now);
            ret = ESP_FAIL; // Update ret to indicate failure after all retries
        } else {
            ESP_LOGI(TAG, "Send ESP-NOW data successfully, channel: %d", now);
        }

    }

    xSemaphoreGive(wifi_connect_lock);
    xSemaphoreGive(g_send_lock);
    ESP_FREE(espnow_data);
    // ESP_ERR_FUNC_RET(esp_wifi_set_channel(primary, second), "Restore wifi channel failed");
    return ret;
}


static esp_err_t espnow_send_forward(espnow_data_t *espnow_data)
{
    ESP_PARAM_CHECK(espnow_data);
    ESP_ERROR_RETURN(!g_espnow_config, ESP_ERR_ESPNOW_NOT_INIT, "ESPNOW is not initialized");

    esp_err_t ret = ESP_OK;
    const char *TAG = "ESP_NOW_SEND_FORWARD";
    TickType_t wait_ticks     = g_espnow_config->send_max_timeout;
    uint32_t start_ticks      = xTaskGetTickCount();

    espnow_frame_head_t *frame_head = &espnow_data->frame_head;
    const uint8_t *dest_addr = espnow_data->dest_addr;

    ESP_LOGI(TAG, "Dst: ["MACSTR"] | Type: %s | Data: [%s] ", MAC2STR(dest_addr),
               espnow_data_type_to_string(espnow_data->type), espnow_data->data);

    bool send_success = false;

    if (xSemaphoreTake(g_send_lock, g_espnow_config->send_max_timeout) != pdPASS) {
        ESP_LOGW(TAG, "Wait Sem fail");
        ESP_FREE(espnow_data);
        return ESP_ERR_TIMEOUT;
    }

    for (int count = 0; count < g_espnow_config->send_retry_num; ++count) {

        ret = esp_now_send(ESPNOW_ADDR_BROADCAST, (uint8_t *)espnow_data, sizeof(espnow_data_t) + espnow_data->size);

        TickType_t write_ticks = (wait_ticks == portMAX_DELAY) ? portMAX_DELAY :
                    xTaskGetTickCount() - start_ticks < wait_ticks ?
                    wait_ticks - (xTaskGetTickCount() - start_ticks) : 0;
        EventBits_t uxBits = xEventGroupWaitBits(g_event_group, SEND_CB_OK | SEND_CB_FAIL,
                                pdTRUE, pdFALSE,  MIN(wait_ticks, g_espnow_config->send_max_timeout));
        if (uxBits & SEND_CB_OK) {
            ESP_LOGI(TAG, "Forward successfully");
            break;
        }

        ESP_LOGE(TAG, "Forward failed, retry: %d", count);
    }

    xSemaphoreGive(g_send_lock);
    ESP_FREE(espnow_data);

    return ret;
}


static esp_err_t espnow_recv_process(espnow_data_t *espnow_data)
{
    ESP_PARAM_CHECK(espnow_data);
    ESP_ERROR_RETURN(!g_espnow_config, ESP_ERR_ESPNOW_NOT_INIT, "ESPNOW is not initialized");

    esp_err_t ret = ESP_OK;

    espnow_frame_head_t *frame_head = &espnow_data->frame_head;
 
    ESP_LOGI(TAG, "Src: ["MACSTR"] | Type: %s | Data: [%s] ", MAC2STR(espnow_data->src_addr),
               espnow_data_type_to_string(espnow_data->type), espnow_data->data);

    size_t size = espnow_data->size;

    uint8_t *data   = ESP_MALLOC(ESPNOW_MAX_DATA_LEN);
    memcpy(data, espnow_data->data, size);

    uint8_t src_addr[6]     = {0};
    memcpy(src_addr, espnow_data->src_addr, 6);

    // if (g_recv_handle[espnow_data->type].handle) {
    //     g_recv_handle[espnow_data->type].handle(src_addr, (void *)data, size, &rx_ctrl);
    // }

    ESP_FREE(espnow_data);
    ESP_FREE(data);
    return ret;
}



static void espnow_main_task(void *pv)
{
    const char *TAG = "ESP_NOW_MAIN_TASK";
    espnow_event_ctx_t evt_data = {0};
    bool loop_continue = true;

    ESP_LOGI(TAG, "Starting");

    if (g_espnow_config && g_espnow_config->qsize) {
        g_espnow_queue = xQueueCreate(g_espnow_config->qsize, sizeof(espnow_event_ctx_t));
        ESP_ERROR_GOTO(!g_espnow_queue, EXIT, "Create espnow event queue fail");

        g_ack_queue = xQueueCreate(4, sizeof(uint32_t *));
        ESP_ERROR_GOTO(!g_ack_queue, EXIT, "Create espnow ack queue fail");

    }

    while (g_espnow_config && loop_continue) {
        if (xQueueReceive(g_espnow_queue, &evt_data, portMAX_DELAY) != pdTRUE) {
            continue;
        }

        switch (evt_data.msg_type) {
            case ESPNOW_MSG_TYPE_STOP:
                loop_continue = false;
                break;

            case ESPNOW_MSG_TYPE_SEND_ACK:
            case ESPNOW_MSG_TYPE_FORWARD:
                if (espnow_send_forward((espnow_data_t *)(evt_data.data)) != ESP_OK) {
                    ESP_LOGI(TAG, "espnow_send_forward failed");
                }
                break;

            case ESPNOW_MSG_TYPE_RECEIVE:
                if (espnow_recv_process((espnow_data_t *)(evt_data.data)) != ESP_OK) {
                    ESP_LOGI(TAG, "espnow_recv_process");
                }
                break;

            default:
                break;
        }
    }

EXIT:
    if (g_espnow_queue) {
        while (xQueueReceive(g_espnow_queue, &evt_data, 0)) {
            ESP_FREE(evt_data.data);
        }

        vQueueDelete(g_espnow_queue);
        g_espnow_queue = NULL;
    }

    if (g_ack_queue) {
        vQueueDelete(g_ack_queue);
        g_ack_queue = NULL;
    }

    ESP_LOGI(TAG, "Exiting");
    vTaskDelete(NULL);
}


esp_err_t espnow_start(const espnow_config_t *config)
{
    esp_err_t ret = ESP_OK;

    wifi_mode_t mode = WIFI_MODE_NULL;
    ESP_ERR_FUNC_RET(esp_wifi_get_mode(&mode), "Get wifi mode failed");
    ESP_ERROR_RETURN(mode != WIFI_MODE_STA, ESP_FAIL, "ESP-NOW only available in STA mode");

    ESP_LOGI(TAG, "ESP-NOW Starting: Version 2.0.0");

    ESP_PARAM_CHECK(config);

    if (g_espnow_config) {
        return ESP_OK;
    }

    g_espnow_config = ESP_MALLOC(sizeof(espnow_config_t));
    memcpy(g_espnow_config, config, sizeof(espnow_config_t));

    /**< Event group for espnow sent cb */
    g_event_group = xEventGroupCreate();
    ESP_ERROR_RETURN(!g_event_group, ESP_FAIL, "Create event group fail");
    
    g_send_lock = xSemaphoreCreateMutex();
    ESP_ERROR_RETURN(!g_send_lock, ESP_FAIL, "Create send semaphore mutex fail");

    if (wifi_connect_lock == NULL) {
        wifi_connect_lock = xSemaphoreCreateMutex();
    }
    ESP_PARAM_CHECK(wifi_connect_lock);
    /**< Initialize ESPNOW function */
    ESP_ERR_FUNC_RET(esp_now_init(), "ESP-NOW Initialize Failed");
    ESP_ERR_FUNC_RET(esp_now_register_send_cb(espnow_send_cb), "Register send callback failed");
    ESP_ERR_FUNC_RET(esp_now_register_recv_cb(espnow_recv_cb), "Register receive callback failed");

    espnow_add_peer(ESPNOW_ADDR_BROADCAST, 0);

    esp_wifi_get_mac(ESP_IF_WIFI_STA, ESPNOW_ADDR_SELF);

    ESP_LOGI(TAG, "Enable main task");
    xTaskCreate(espnow_main_task, "espnow_main", 
                                    1024 * 4, 
                                    NULL, 
                                    1, NULL);

    return ESP_OK;
}


esp_err_t espnow_stop(void)
{
    ESP_ERROR_RETURN(!g_espnow_config, ESP_ERR_ESPNOW_NOT_INIT, "ESPNOW is not initialized");

    /**< De-initialize ESPNOW function */
    ESP_ERROR_CHECK(esp_now_unregister_recv_cb());
    ESP_ERROR_CHECK(esp_now_unregister_send_cb());
    ESP_ERROR_CHECK(esp_now_deinit());

    if (queue_over_write(ESPNOW_MSG_TYPE_STOP, NULL, 0, NULL, 0) != pdPASS) {
        ESP_LOGW(TAG, "[%s, %d] Send queue failed", __func__, __LINE__);
    }

    vSemaphoreDelete(g_send_lock);
    g_send_lock = NULL;

    vEventGroupDelete(g_event_group);
    g_event_group = NULL;

    ESP_FREE(g_espnow_config);
    g_espnow_config = NULL;

    return ESP_OK;
}