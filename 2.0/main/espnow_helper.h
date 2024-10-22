#pragma once

#include "esp_now.h"
#include "mem_helper.h"
#include "my_utils.h"

#ifdef __cplusplus
extern "C" {
#endif /**< _cplusplus */


#define ESPNOW_MAX_DATA_LEN                 (230)

#define ESPNOW_ADDR_LEN                     (6)
#define ESPNOW_ADDR_IS_EMPTY(addr)          (((addr)[0] | (addr)[1] | (addr)[2] | (addr)[3] | (addr)[4] | (addr)[5]) == 0x0)
#define ESPNOW_ADDR_IS_BROADCAST(addr)      (((addr)[0] & (addr)[1] & (addr)[2] & (addr)[3] & (addr)[4] & (addr)[5]) == 0xFF)
#define ESPNOW_ADDR_IS_SELF(addr)           !memcmp(addr, ESPNOW_ADDR_SELF, 6)
#define ESPNOW_ADDR_IS_EQUAL(addr1, addr2)  !memcmp(addr1, addr2, 6)

typedef uint8_t espnow_addr_t[6];
typedef uint8_t espnow_group_t[6];

/**
 * @brief The channel on which the device sends packets
 */
#define ESPNOW_CHANNEL_CURRENT               0x0   /**< Only in the current channel */
#define ESPNOW_CHANNEL_ALL                   0x0f  /**< All supported channels */

#define ESPNOW_RETRANSMIT_MAX_COUNT          0x1f  /**< Maximum number of retransmissions */
#define ESPNOW_FORWARD_MAX_COUNT             0x1f  /**< Maximum number of forwards */

/**
 * @brief Initialize the configuration of espnow
 */
typedef struct {
    bool forward_enable             : 1;    /**< Forward when packets are received */
    bool master_channel             : 1;    /**< Forward data packet with exchange channel */
    uint8_t reserved1               : 6;    /**< Reserved */
    uint8_t qsize;                          /**< Size of packet buffer queue */
    uint8_t send_retry_num;                 /**< Number of retransmissions */
    uint32_t send_max_timeout;              /**< Maximum timeout */
} espnow_config_t;

#define ESPNOW_INIT_CONFIG_DEFAULT() { \
    .forward_enable = 1, \
    .master_channel = 0, \
    .reserved1 = 0,   \
    .qsize = 32, \
    .send_retry_num = 4, \
    .send_max_timeout = pdMS_TO_TICKS(5000)\
    }

/**
 * @brief Divide ESP-NOW data into multiple pipes
 */
typedef enum {
    ESPNOW_DATA_TYPE_ACK,            /**< For reliable data transmission */
    ESPNOW_DATA_TYPE_FORWARD,        /**< Set to forward packets */
    ESPNOW_DATA_TYPE_GROUP,          /**< Send a packet that sets the group type */
    ESPNOW_DATA_TYPE_CONTROL,       /**< Binding or unbinding packet */
    ESPNOW_DATA_TYPE_DEBUG,      /**< Equipment debugging log packet */
    ESPNOW_DATA_TYPE_DATA,           /**< User-defined use */
    ESPNOW_DATA_TYPE_MAX,
} espnow_data_type_t;


/**
 * @brief Frame header of espnow
 */
typedef struct espnow_frame_head_s {
    uint16_t magic;                    /**< Unique identifier of each packet. Packets with the same identifier will be filtered. 0: a random number */
    uint8_t channel              : 4;  /**< Set the channel where the packet is sent, ESPNOW_CHANNEL_CURRENT or ESPNOW_CHANNEL_ALL */
    uint8_t reserved             : 4;  /**< Reserved */
    bool forward                 : 1;  /**< Packet sent in forward mode or unicast mode */
    bool group                   : 1;  /**< Only the group set as forward transmission mode is valid */
    bool ack                     : 1;  /**< Wait for the receiving device to return ack to ensure transmission reliability */
    uint8_t forward_ttl          : 5;  /**< Number of hops in data transfer */
} __attribute__((packed)) espnow_frame_head_t;

#define ESPNOW_FRAME_CONFIG_DEFAULT() \
    { \
        .forward = false, \
        .ack = false, \
        .forward_ttl = 10, \
    }

typedef struct {
    bool join;
    uint8_t master_id[6];
    uint8_t group_id[6];
    uint8_t master_channel;
    uint8_t addrs_num;
    uint8_t addrs_list[0][6];
} espnow_group_info_t;


typedef struct {
    uint8_t type    : 4;
    uint8_t         : 4;
    uint8_t size;
    espnow_frame_head_t frame_head;
    uint8_t dest_addr[6];
    uint8_t src_addr[6];
    uint8_t data[0];
} __attribute__((packed)) espnow_data_t;


typedef enum {
    ESPNOW_MSG_TYPE_SEND_ACK,
    ESPNOW_MSG_TYPE_RECV_ACK,
    ESPNOW_MSG_TYPE_FORWARD,
    ESPNOW_MSG_TYPE_RECEIVE,
    ESPNOW_MSG_TYPE_STOP,
} espnow_msg_type_t;

typedef struct {
    espnow_msg_type_t msg_type;
    size_t data_len;
    void *data;
    void *handle;
} espnow_event_ctx_t;


/**
 * @brief   ESP-NOW data receive callback function for the corresponding data type
 *
 * @param[in]  src_addr  peer MAC address
 * @param[in]  data  received data
 * @param[in]  size  length of received data
 * @param[in]  rx_ctrl  received packet radio metadata header
 *
 * @return
 *    - ESP_OK
 *    - ESP_ERR_INVALID_ARG
 */
typedef esp_err_t (*data_handler_t)(uint8_t *src_addr, void *data,
                                   size_t size, wifi_pkt_rx_ctrl_t *rx_ctrl);


/**
 * @brief Add a peer to the ESP-NOW peer list.
 * 
 * @param addr The MAC address of the peer to add.
 * @return esp_err_t ESP_OK on success, ESP_FAIL on error.
 */
esp_err_t espnow_add_peer(const espnow_addr_t addr, uint8_t channel);

/**
 * @brief Delete a peer from the ESP-NOW peer list.
 * 
 * @param addr The MAC address of the peer to delete.
 * @return esp_err_t ESP_OK on success, ESP_FAIL on error.
 */
esp_err_t espnow_del_peer(const espnow_addr_t addr);

/**
 * @brief Send data using ESP-NOW.
 * 
 * @param type The type of data being sent.
 * @param dest_addr The destination MAC address.
 * @param data Pointer to the data to send.
 * @param size Size of the data to send.
 * @param forward Whether the message should be broadcasted.
 * @return esp_err_t ESP_OK on success, ESP_FAIL on error.
 */
esp_err_t espnow_send(espnow_data_type_t type, const espnow_addr_t dest_addr, const void *data, size_t size, bool forward);

/**
 * @brief Set the ESP-NOW group configuration.
 * 
 * @param addrs_list List of MAC addresses in the group.
 * @param addrs_num Number of addresses in the list.
 * @param group_id The group ID.
 * @param join Whether to join or leave the group.
 * @return esp_err_t ESP_OK on success, ESP_FAIL on error.
 */
esp_err_t espnow_set_group(const uint8_t addrs_list[][ESPNOW_ADDR_LEN], size_t addrs_num, const uint8_t group_id[ESPNOW_ADDR_LEN], bool join);

/**
 * @brief Start the ESP-NOW service with the given configuration.
 * 
 * @param config Pointer to the ESP-NOW configuration structure.
 * @return esp_err_t ESP_OK on success, ESP_FAIL on error.
 */
esp_err_t espnow_start(const espnow_config_t *config);

/**
 * @brief Stop the ESP-NOW service.
 * 
 * @return esp_err_t ESP_OK on success, ESP_FAIL on error.
 */
esp_err_t espnow_stop(void);


#ifdef __cplusplus
}
#endif /**< _cplusplus */