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
#include "my_utils.h"
#include "ws_helper.h"

#include "mqtt_client.h"

#define MAX_TOPIC_LENGTH                 (15)
#define MAX_TOPIC_PATH_LENGTH            (30)

#define TOPIC_BASEPATH                   ("esp32")

#define MQTT_TOPIC_RECV_CMD              ("recv_cmd")
#define MQTT_TOPIC_SEND_CMD              ("send_cmd")
#define MQTT_TOPIC_SYNC_CMD              ("sync_cmd")


/**
 * @brief Start the MQTT helper with the provided configuration.
 *
 * This function initializes the MQTT client with the given configuration and starts the client.
 * It also registers the MQTT event handler to handle MQTT events.
 *
 * @param mqtt_cfg The configuration for the MQTT client.
 * @return esp_err_t Returns ESP_OK if the MQTT client started successfully, else returns an error code.
 */
esp_err_t mqtt_helper_start(esp_mqtt_client_config_t mqtt_cfg);

/**
 * @brief Stop the MQTT helper.
 *
 * This function stops the MQTT client and disconnects it from the broker.
 *
 * @return esp_err_t Returns ESP_OK if the MQTT client stopped successfully, else returns an error code.
 */
esp_err_t mqtt_helper_stop(void);

/**
 * @brief Enqueue messages for multiple topics from the provided topic list.
 *
 * This function constructs the full topic path for each topic in the list by combining the base path
 * with the topic filter. It then enqueues the message for each topic with the specified QoS level.
 *
 * @param base_path The base path for the topics.
 * @param topic_list The list of topics to enqueue messages for.
 * @param len The number of topics in the list.
 * @param msg The message to enqueue.
 * @param qos The Quality of Service level for the message.
 * 
 * @return int8_t Returns 0 if all messages were enqueued successfully, else returns -2.
 */
int8_t mqtt_enqueue_from_list(const char* base_path, const esp_mqtt_topic_t* topic_list, size_t len, char* msg, int qos);

/**
 * @brief Subscribe to multiple topics from the provided topic list.
 *
 * This function constructs the full topic path for each topic in the list by combining the base path
 * with the topic filter. It then subscribes to each topic.
 *
 * @param base_path The base path for the topics.
 * @param topic_list The list of topics to subscribe to.
 * @param len The number of topics in the list.
 * 
 * @return int8_t Returns 0 if all topics were subscribed to successfully, else returns -2.
 */
int8_t mqtt_subscribe_from_list(const char* base_path, const esp_mqtt_topic_t* topic_list, size_t len);

/**
 * @brief Unsubscribe from multiple topics from the provided topic list.
 *
 * This function constructs the full topic path for each topic in the list by combining the base path
 * with the topic filter. It then unsubscribes from each topic.
 *
 * @param base_path The base path for the topics.
 * @param topic_list The list of topics to unsubscribe from.
 * @param len The number of topics in the list.
 * 
 * @return int8_t Returns 0 if all topics were unsubscribed from successfully, else returns -2.
 */
int8_t mqtt_unsubscribe_from_list(const char* base_path, const esp_mqtt_topic_t* topic_list, size_t len);


bool mqtt_helper_is_connected();
