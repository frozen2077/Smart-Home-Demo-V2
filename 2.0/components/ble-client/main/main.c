#include "esp_log.h"
#include "nvs_flash.h"
/* BLE */
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "host/ble_hs.h"
#include "host/util/util.h"
#include "console/console.h"
#include "store/config/ble_store_config.h"
#include "services/gap/ble_svc_gap.h"
#include "services/gatt/ble_svc_gatt.h"

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

#include "modlog/modlog.h"
#include "esp_central.h"

#include "helpers/nvs_helper.c"
#include "helpers/mem_helper.c"
#include "my_utils.h"

/* 16 Bit SPP Service UUID */
#define BLE_SVC_SPP_UUID16                                  0xABF0

/* 16 Bit SPP Service Characteristic UUID */
#define BLE_CHR_RW                                          0xABF1
#define BLE_CHR_NOTIFY                                      0xABF2


ble_uuid_t *SVC_UUID  = BLE_UUID16_DECLARE(0xABF0);
ble_uuid_t *CHR_UUID  = BLE_UUID16_DECLARE(0xABF1);
ble_uuid_t *NOTI_UUID = BLE_UUID16_DECLARE(0xABF2);

static SemaphoreHandle_t read_sem, write_sem;

static uint16_t rw_handle[4];
static ble_addr_t connected_addr[4];

static uint16_t conn_handle;

static bool write_enable = false;

static void ble_gap_event_handler(struct ble_gap_event *event, void *arg);
static void ble_scan(void);
const char* ble_err2str(int err_code);
static void ble_subscribe(const struct peer *peer, ble_uuid_t *svc_uuid, 
                          ble_uuid_t *chr_uuid, uint8_t op);
static void ble_write(uint16_t conn_handle, uint16_t val_handle);
static void ble_read(uint16_t conn_handle, uint16_t chr_handle);
void ble_store_config_init(void);



#define BLE_FUNC_ERR_RET(con, format, ...) do { \
    int err = (con); \
        if ((err) != 0) { \
            if(*format != '\0') \
                ESP_LOGW(TAG, "[%s, %d] <%s> " format, __func__, __LINE__, ble_err2str(err), ##__VA_ARGS__); \
            return; \
        } \
    } while(0) 

/**
 * @brief Subscribes or unsubscribes to notifications or indications for a specific characteristic on a peer device.
 * 
 * This function finds the characteristic and its client characteristic configuration descriptor (CCCD) on the peer device
 * based on the provided service and characteristic UUIDs. Depending on the operation parameter, it then writes the appropriate
 * value to the CCCD to subscribe to notifications, subscribe to indications, or unsubscribe from both.
 * 
 * @param peer Pointer to the peer device structure.
 * @param svc_uuid UUID of the service containing the characteristic.
 * @param chr_uuid UUID of the characteristic to subscribe to or unsubscribe from.
 * @param op Operation to perform: 0 for unsubscribe, 1 for subscribe to indications, 2 for subscribe to notifications.
 */
static void ble_subscribe(const struct peer *peer, ble_uuid_t *svc_uuid, 
                          ble_uuid_t *chr_uuid, uint8_t op)
{
    const char* TAG = "BLE_SUBSCRIBE";
    uint8_t disable[2]         = {0, 0};
    uint8_t enable_indicate[2] = {0, 1};
    uint8_t enable_notify[2]   = {1, 0};

    struct peer_chr* chr = peer_chr_find_uuid(peer, svc_uuid, chr_uuid);
    if (chr == NULL) {
        ESP_LOGE(TAG, "Error: Peer doesn't support the NOTIFY "
                    "characteristic (0x000a) ");
        return;
    }
    struct peer_dsc* dsc = peer_dsc_find_uuid(peer,
                                svc_uuid,
                                chr_uuid,
                                BLE_UUID16_DECLARE(BLE_GATT_DSC_CLT_CFG_UUID16));
    if (dsc == NULL) {
        ESP_LOGE(TAG, "Error: Peer lacks a CCCD for the Notify "
                    "Status characteristic\n");
        return;
    }
    
    switch(op) {
        case 0:
            BLE_FUNC_ERR_RET(ble_gattc_write_flat(peer->conn_handle, dsc->dsc.handle,
                                  disable, sizeof disable, NULL, NULL),
                            "Failed to unsubscribe from >%04x<", dsc->dsc.handle);
            ESP_LOGI(TAG, "Unsubscribed from >0x%04x<", dsc->dsc.handle);
            break;
        case 1:
            BLE_FUNC_ERR_RET(ble_gattc_write_flat(peer->conn_handle, dsc->dsc.handle,
                                  enable_indicate, sizeof enable_indicate, NULL, NULL),
                            "Failed to subscribe to indications of >%04x<", dsc->dsc.handle);
            ESP_LOGI(TAG, "Subscribed to indications of >0x%04x<", dsc->dsc.handle);
            break;
        case 2:
            BLE_FUNC_ERR_RET(ble_gattc_write_flat(peer->conn_handle, dsc->dsc.handle,
                                  enable_notify, sizeof enable_notify, NULL, NULL),
                            "Failed to subscribe to notifications of >%04x<", dsc->dsc.handle);
            ESP_LOGI(TAG, "Subscribed to notifications of >0x%04x<", dsc->dsc.handle);
            break;            
        default:
            ESP_LOGE(TAG, "Invalid operation code for subscription");
            return;
    }

}


static void ble_write(uint16_t conn_handle, uint16_t chr_handle)
{
    const char* TAG = "BLE_WRITE";
    char value[100] = {0};
    static int num = 0;
    uint8_t rc = 0;
    struct os_mbuf *om;

    xSemaphoreTake(write_sem, portMAX_DELAY);
    // for(int i = 0; rc == BLE_HS_ENOMEM && i < 3; i++) {
    //     rc = ble_gattc_write_flat(conn_handle, chr_handle, &value, sizeof value, NULL, NULL);
    //     if (rc == BLE_HS_ENOMEM) vTaskDelay(5);
    // }

    if (os_msys_num_free() >= 2) {
        do {
            memset(value, 0, sizeof value);
            snprintf(value, sizeof value, "I'm gonna fuck you up! Fallout 5 is best: %d", num++);          
            om = ble_hs_mbuf_from_flat(value, sizeof value);
            if (om == NULL) {
                ESP_LOGE(TAG, "No MBUFs available from pool, retry..");
                vTaskDelay(100 / portTICK_PERIOD_MS);
            }
        } while (om == NULL);

        BLE_FUNC_ERR_RET(ble_gattc_write_no_rsp(conn_handle, chr_handle, om),
                        "Error while writing");
        ESP_LOGI(TAG, "Successfully wrote conn [%d] chr [%04x] < [%s]",
                        conn_handle, chr_handle, value);
    } else {
        ESP_LOGE(TAG, "Not enough OS_MBUFs available");
    }

    xSemaphoreGive(write_sem);

}


void ble_write_task(void *pvParameters) {
    const char* TAG = "BLE_WRITE_TASK";
    // uint16_t conn_handle = *((uint16_t*)pvParameters); 
    // uint16_t val_handle = 0xABF1; 

    while (1) {
        if (!write_enable) {
            vTaskDelay(1000 / portTICK_PERIOD_MS);
            continue;
        }

        ESP_LOGI(TAG, "Reading...");
        ble_read(conn_handle, rw_handle[conn_handle]);
        vTaskDelay(pdMS_TO_TICKS(3000)); // Wait for 5 seconds
        ESP_LOGI(TAG, "Sending BLE message...");
        ble_write(conn_handle, rw_handle[conn_handle]);
        vTaskDelay(pdMS_TO_TICKS(3000)); // Wait for 5 seconds
    }
}


static int ble_on_read(uint16_t conn_handle,
                    const struct ble_gatt_error *error,
                    struct ble_gatt_attr *attr,
                    void *arg)
{
    const char* TAG = "BLE_ON_READ";
    char* msg = NULL;
    if (error->status == 0) {
        msg = (char *)calloc(OS_MBUF_PKTLEN(attr->om) + 1, sizeof(char));
        if (msg != NULL) {
            os_mbuf_copydata(attr->om, 0, OS_MBUF_PKTLEN(attr->om), msg);
            ESP_LOGI(TAG, "Read from chr_handle [%04x]: [%s] | len: [%d]",
                        attr->handle, msg, OS_MBUF_PKTLEN(attr->om));
            free(msg);
        } else {
            ESP_LOGE(TAG, "Failed to allocate memory for read message");
        }        
    } else {
        ESP_LOGE(TAG, "Read failed >%d<", error->status);
    }

    xSemaphoreGive(read_sem);
    return error->status;
}

static void ble_read(uint16_t conn_handle, uint16_t chr_handle)
{
    const char* TAG = "BLE_READ";
    if (xSemaphoreTake(read_sem, portMAX_DELAY) == pdTRUE) {

        int rc = ble_gattc_read(conn_handle, chr_handle,
                            ble_on_read, NULL);
        if (rc != 0) {
            ESP_LOGE(TAG, "Failed to read characteristic");
            xSemaphoreGive(read_sem);
        }
    } else {
        ESP_LOGE(TAG, "Failed to take read semaphore");
    }
}


static void ble_set_handle(const struct peer *peer)
{
    const char* TAG = "BLE_SET_HANDLE";
    const struct peer_chr *chr = peer_chr_find_uuid(peer, SVC_UUID, CHR_UUID);
    rw_handle[peer->conn_handle] = chr->chr.val_handle;
    ESP_LOGI(TAG, "Set handle: Conn [%d] | Char [%04x]", peer->conn_handle, chr->chr.val_handle);
}

static void ble_disc_complete_cb(const struct peer *peer, int status, void *arg)
{
    const char* TAG = "BLE_DISC_CMPL";

    if (status != 0) {
        ESP_LOGE(TAG, "Error: Service discovery failed; status=%d "
                    "conn_handle=%d\n", status, peer->conn_handle);
        ble_gap_terminate(peer->conn_handle, BLE_ERR_REM_USER_CONN_TERM);
        return;
    }

    ESP_LOGI(TAG, "Service discovery complete; status=%d "
                "conn_handle=%d\n", status, peer->conn_handle);

    ble_set_handle(peer);

    write_enable = true;

    // ble_scan();
}

static void ble_connect_handler(const struct ble_gap_disc_desc *disc, ble_uuid_t *svc_uuid)
{
    const char* TAG = "BLE_CONNECT_HANDLER";
    bool should_connect = false;

    for (int i = 0; i <= CONFIG_BT_NIMBLE_MAX_CONNECTIONS; i++) {
        if (memcmp(&connected_addr[i].val, disc->addr.val, PEER_ADDR_VAL_SIZE) == 0) {
            ESP_LOGI(TAG, "Device already connected");
            return;
        }
    }

    if (disc->event_type != BLE_HCI_ADV_RPT_EVTYPE_ADV_IND &&
        disc->event_type != BLE_HCI_ADV_RPT_EVTYPE_DIR_IND) {
        return;
    }

    struct ble_hs_adv_fields fields;
    BLE_FUNC_ERR_RET(ble_hs_adv_parse_fields(&fields, disc->data, disc->length_data), 
                    "Failed to parse fields");

    for (int i = 0; i < fields.num_uuids16; i++) {
        if (ble_uuid_cmp(&fields.uuids16[i].u, svc_uuid) == 0) {
            should_connect = true;
            break;
        }
    }

    if (should_connect) {
        BLE_FUNC_ERR_RET(ble_gap_disc_cancel(), "Failed to cancel scan");

        uint8_t own_addr_type;
        BLE_FUNC_ERR_RET(ble_hs_id_infer_auto(0, &own_addr_type), 
                                        "error determining address type");

        BLE_FUNC_ERR_RET(ble_gap_connect(own_addr_type, &disc->addr, 30000, NULL,
                                        ble_gap_event_handler, NULL), 
                                        "Failed to connect to device: %s",
                                        addr_str(disc->addr.val));
    } else {
        // ESP_LOGI(TAG, "Not the device we are looking for");
    }

}

static void ble_gap_event_handler(struct ble_gap_event *event, void *arg)
{
    const char* TAG = "BLE_GAP_HANDLER";
    struct ble_gap_conn_desc desc;
    struct ble_hs_adv_fields fields;
    int ret;

    switch (event->type) {
    case BLE_GAP_EVENT_DISC:

        ESP_LOGI(TAG, "Found [%s] | Rssi: [%d]", addr_str(event->disc.addr.val), event->disc.rssi);

        BLE_FUNC_ERR_RET(
            ble_hs_adv_parse_fields(&fields, event->disc.data,
                                     event->disc.length_data), 
                                    "error parsing advertisement fields");
        // print_adv_fields(&fields);

        ble_connect_handler(&event->disc, SVC_UUID);
        return;

    case BLE_GAP_EVENT_CONNECT:

        if (event->connect.status == 0) {

            ESP_LOGI(TAG, ">>> Connection established <<<");
            conn_handle = event->connect.conn_handle;

            ble_gap_conn_find(conn_handle, &desc);

            memcpy(&connected_addr[conn_handle].val, desc.peer_id_addr.val,
                   PEER_ADDR_VAL_SIZE);
            print_conn_desc(&desc);

            BLE_FUNC_ERR_RET(peer_add(conn_handle), "Failed to add peer");

            BLE_FUNC_ERR_RET(peer_disc_all(conn_handle,
                               ble_disc_complete_cb, NULL), 
                               "Failed to discover services");

        } else {
            ESP_LOGI(TAG, "Error: Connection failed; status=%d\n",
                            event->connect.status);
            ble_scan();
        }

        return;

    case BLE_GAP_EVENT_DISCONNECT:
        /* Connection terminated. */
        ESP_LOGI(TAG, "disconnect; reason=%d ", event->disconnect.reason);
        print_conn_desc(&event->disconnect.conn);

        write_enable = false;

        uint16_t conn_handle = event->disconnect.conn.conn_handle;
        memset(&connected_addr[conn_handle].val, 0, PEER_ADDR_VAL_SIZE);
        rw_handle[conn_handle] = 0;

        peer_delete(conn_handle);

        ble_scan();
        return;

    case BLE_GAP_EVENT_DISC_COMPLETE:
        ESP_LOGI(TAG, "discovery complete; reason=%d\n",
                    event->disc_complete.reason);
        return;

    case BLE_GAP_EVENT_NOTIFY_RX:

        char* msg = (char *)calloc(OS_MBUF_PKTLEN(event->notify_rx.om) + 1, sizeof(char));
        os_mbuf_copydata(event->notify_rx.om, 0, OS_MBUF_PKTLEN(event->notify_rx.om), msg);
        ESP_LOGI(TAG, "Rcv: >%s< | Msg: [%s] | conn: [%d|%d] | len: [%d]",
                    event->notify_rx.indication ? "I" : "N", msg,
                    event->notify_rx.conn_handle,
                    event->notify_rx.attr_handle,
                    OS_MBUF_PKTLEN(event->notify_rx.om));
        free(msg);
        return;

    case BLE_GAP_EVENT_MTU:
        ESP_LOGI(TAG, "mtu update event; conn_handle=%d cid=%d mtu=%d\n",
                    event->mtu.conn_handle,
                    event->mtu.channel_id,
                    event->mtu.value);
        return;

    default:
        return;
    }
}


static void ble_scan(void)
{
    const char* TAG = "BLE_SCAN";
    uint8_t own_addr_type;
    struct ble_gap_disc_params disc_params;
    int ret;

    /* Figure out address to use while advertising (no privacy for now) */
    BLE_FUNC_ERR_RET(
        ble_hs_id_infer_auto(0, &own_addr_type), "error determining address type");
    
    disc_params.filter_duplicates = 1;
    disc_params.passive = 1;
    disc_params.itvl = 0;
    disc_params.window = 0;
    disc_params.filter_policy = 0;
    disc_params.limited = 0;

    BLE_FUNC_ERR_RET(
        ble_gap_disc(own_addr_type, BLE_HS_FOREVER, &disc_params,
                      ble_gap_event_handler, NULL), 
                    "Error initiating GAP discovery procedure");
}

static void hs_reset_cb(int reason)
{
    ESP_LOGI("BLE_HOST_RESET", "Resetting state; reason=%d\n", reason);
}

static void hs_sync_cb(void)
{
    /* Make sure we have proper identity address set (public preferred) */
    ble_hs_util_ensure_addr(0);

    // uint16_t mtu = 0;
    // uint16_t timeout = 0;

    // BLE_FUNC_ERR_RET(ble_gap_write_sugg_def_data_len(251, 1024),
    //                 "Failed to set suggested data length");  
    // BLE_FUNC_ERR_RET(ble_gap_read_sugg_def_data_len(&mtu, &timeout),
    //                 "Failed to set suggested data length");
    // ESP_LOGI(TAG, "MTU: [%d] | Timeout: [%d]", mtu, timeout);

    ble_scan();
}

void hs_task(void *pv)
{
    ESP_LOGI("BLE_HOST_TASK", "Task Started");
    xTaskCreate(ble_write_task, "BLE Write Task", 2048, NULL, 1, NULL);
    nimble_port_run();
    nimble_port_freertos_deinit(); 
}

void app_main(void)
{
    ESP_ERROR_CHECK(nvs_helper_init());

    if (read_sem == NULL) {
        read_sem = xSemaphoreCreateCounting(1, 1);
    }
    if (write_sem == NULL) {
        write_sem = xSemaphoreCreateMutex();
    }

    ESP_ERROR_CHECK(nimble_port_init());

    /* Initialize data structures to track connected peers. */
    ESP_ERROR_CHECK(peer_init(5, 8, 8, 8));

    /* Set the default device name. */
    ESP_ERROR_CHECK(ble_svc_gap_device_name_set("JOY-CLIENT"));

    /* XXX Need to have template for store */
    /* Configure the host. */
    ble_hs_cfg.reset_cb = hs_reset_cb;
    ble_hs_cfg.sync_cb  = hs_sync_cb;
    ble_hs_cfg.store_status_cb = ble_store_util_status_rr;    
    ble_store_config_init();

    nimble_port_freertos_init(hs_task);

}

const char* ble_err2str(int err_code) {
    switch (err_code) {
        case 0: return "E_OK";
        case 1: return "E_AGAIN";
        case 2: return "E_ALREADY";
        case 3: return "E_INVAL";
        case 4: return "E_MSGSIZE";
        case 5: return "E_NOENT";
        case 6: return "E_NOMEM";
        case 7: return "E_NOTCONN";
        case 8: return "E_NOTSUP";
        case 9: return "E_APP";
        case 10: return "E_BADDATA";
        case 11: return "E_OS";
        case 12: return "E_CONTROLLER";
        case 13: return "E_TIMEOUT";
        case 14: return "E_DONE";
        case 15: return "E_BUSY";
        case 16: return "E_REJECT";
        case 17: return "E_UNKNOWN";
        case 18: return "E_ROLE";
        case 19: return "E_TIMEOUT_HCI";
        case 20: return "E_NOMEM_EVT";
        case 21: return "E_NOADDR";
        case 22: return "E_NOTSYNCED";
        case 23: return "E_AUTHEN";
        case 24: return "E_AUTHOR";
        case 25: return "E_ENCRYPT";
        case 26: return "E_ENCRYPT_KEY_SZ";
        case 27: return "E_STORE_CAP";
        case 28: return "E_STORE_FAIL";
        case 29: return "E_PREEMPTED";
        case 30: return "E_DISABLED";
        case 31: return "E_STALLED";
        default: return "N/A";
    }
}


