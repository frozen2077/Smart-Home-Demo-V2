#include "esp_log.h"
#include "esp_mac.h"
#include "nvs_flash.h"
/* BLE */
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "host/ble_hs.h"
#include "host/util/util.h"
#include "services/gap/ble_svc_gap.h"
#include "services/gatt/ble_svc_gatt.h"
#include "esp_peripheral.h"

#include "my_utils.h"
#include "helpers/nvs_helper.c"
#include "helpers/mem_helper.c"


/* 16 Bit SPP Service UUID */
#define BLE_SVC_SPP_UUID16                                  0xABF0

/* 16 Bit SPP Service Characteristic UUID */
#define BLE_CHR_RW                                          0xABF1
#define BLE_CHR_NOTIFY                                      0xABF2

struct ble_hs_cfg;
struct ble_gatt_register_ctxt;

static SemaphoreHandle_t notify_sem;

static uint8_t own_addr_type;
static volatile char rw_val[500];
uint16_t notify_handle;
uint16_t conn_handle;

bool notify_state = false;

void ble_store_config_init(void);

static void ble_gap_event_handler(struct ble_gap_event *event, void *arg);
static void ble_advertise(void);
static const char* ble_gap_event2str(uint8_t event_type);
static const char* ble_err2str(int err_code);
static void ble_access_handler(uint16_t conn_handle, uint16_t attr_handle,
                              struct ble_gatt_access_ctxt *ctxt,
                              void *arg);
static int ble_update_conn_params(uint16_t conn_handle,
                            uint16_t minInterval, uint16_t maxInterval,
                            uint16_t latency, uint16_t timeout);


#define BLE_FUNC_ERR_RET(con, format, ...) do { \
    int err = (con); \
        if ((err) != 0) { \
            if(*format != '\0') \
                ESP_LOGW(TAG, "[%s, %d] <%s> " format, __func__, __LINE__, ble_err2str(err), ##__VA_ARGS__); \
            return; \
        } \
    } while(0) 

/* Define new custom service */
static const struct ble_gatt_svc_def svr_defs[] = {
    {
        /*** Service: SPP */
        .type = BLE_GATT_SVC_TYPE_PRIMARY,
        .uuid = BLE_UUID16_DECLARE(BLE_SVC_SPP_UUID16),
        .characteristics = (struct ble_gatt_chr_def[]){
            {
                /* Support SPP service */
                .uuid = BLE_UUID16_DECLARE(BLE_CHR_RW),
                .access_cb = ble_access_handler,
                .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_WRITE,
            },
            {
                .uuid = BLE_UUID16_DECLARE(BLE_CHR_NOTIFY),
                .access_cb = ble_access_handler,
                .val_handle = &notify_handle,
                .flags = BLE_GATT_CHR_F_NOTIFY,
            }, {0}           

        },
    }, {0}
};


static void ble_access_handler(uint16_t conn_handle, uint16_t attr_handle,
                              struct ble_gatt_access_ctxt *ctxt,
                              void *arg)
{
    const char* TAG = "BLE_ACCESS_HANDLER";
    static uint16_t num = 0;
    ESP_LOGI(TAG, "Operation >%d< | uuid >0x%04x<", ctxt->op, ble_uuid_u16(ctxt->chr->uuid));
    switch (ble_uuid_u16(ctxt->chr->uuid)) {
    case BLE_CHR_RW:
        if (ctxt->op == BLE_GATT_ACCESS_OP_WRITE_CHR) {

            char* msg = (char *)calloc(OS_MBUF_PKTLEN(ctxt->om) + 1, sizeof(char));
            os_mbuf_copydata(ctxt->om, 0, OS_MBUF_PKTLEN(ctxt->om), msg);
            ESP_LOGI(TAG, "Client Write: [%s] | len: [%d]",
                        msg, OS_MBUF_PKTLEN(ctxt->om));
            free(msg);            

        } 
        else if (ctxt->op == BLE_GATT_ACCESS_OP_READ_CHR) { 

            char msg[50] = {0};
            snprintf(msg, sizeof(msg), "Burn this city to the ground!: %d", num++);
            int rc = os_mbuf_append(ctxt->om, &msg, sizeof(msg));                    
        }
        return;
    case BLE_CHR_NOTIFY:
        return;
    default:
        return;
    }
}



static void ble_advertise(void)
{
    const char *TAG = "BLE_ADVERTISE";
    struct ble_gap_adv_params adv_params;
    struct ble_hs_adv_fields fields;

    memset(&fields, 0, sizeof fields);

    fields.flags = BLE_HS_ADV_F_DISC_GEN |
                   BLE_HS_ADV_F_BREDR_UNSUP;

    fields.tx_pwr_lvl_is_present = 1;
    fields.tx_pwr_lvl = BLE_HS_ADV_TX_PWR_LVL_AUTO;

    const char *name = ble_svc_gap_device_name();
    fields.name = (uint8_t *)name;
    fields.name_len = strlen(name);
    fields.name_is_complete = 1;

    fields.uuids16 = (ble_uuid16_t[]) {
        BLE_UUID16_INIT(BLE_SVC_SPP_UUID16)
    };
    fields.num_uuids16 = 1;
    fields.uuids16_is_complete = 1;

    BLE_FUNC_ERR_RET(ble_gap_adv_set_fields(&fields),
                        "set advertisement data failed");

    memset(&adv_params, 0, sizeof adv_params);
    adv_params.conn_mode = BLE_GAP_CONN_MODE_UND;
    adv_params.disc_mode = BLE_GAP_DISC_MODE_GEN;
    BLE_FUNC_ERR_RET(ble_gap_adv_start(own_addr_type, NULL, BLE_HS_FOREVER,
                           &adv_params, ble_gap_event_handler, NULL),
                        "start advertisement failed");
}

static void ble_print_conn_desc(struct ble_gap_conn_desc *desc)
{
    const char* TAG = "BLE_PRINT_CONN_DESC";
    ESP_LOGI(TAG, "peer_id_addr_type=%d peer_id_addr="MACSTR,
                desc->peer_id_addr.type, MAC2STR(desc->peer_id_addr.val));
    ESP_LOGI(TAG, "conn_itvl=%d conn_latency=%d supervision_timeout=%d "
                "encrypted=%d authenticated=%d bonded=%d",
                desc->conn_itvl, desc->conn_latency,
                desc->supervision_timeout,
                desc->sec_state.encrypted,
                desc->sec_state.authenticated,
                desc->sec_state.bonded);
}

void ble_gatt_mtu_cb(uint16_t conn_handle,
                    const struct ble_gatt_error *error,
                    uint16_t mtu, void *arg)
{
    ESP_LOGW("BLE_GATT_MTU_FN", "MTU: [%d]", mtu);
}


static void ble_gap_event_handler(struct ble_gap_event *event, void *arg)
{
    const char* TAG = "BLE_GAP_HANDLER";
    struct ble_gap_conn_desc desc;
    struct ble_hs_adv_fields fields;

    switch (event->type) {
    case BLE_GAP_EVENT_CONNECT:

        if (event->connect.status == 0) {

            ESP_LOGI(TAG, "Connection established ");
            conn_handle = event->connect.conn_handle;

            ble_gap_conn_find(conn_handle, &desc);
            ble_print_conn_desc(&desc);     

            BLE_FUNC_ERR_RET(ble_gattc_exchange_mtu(conn_handle, ble_gatt_mtu_cb, NULL),
                            "Failed to exchange MTU");

            ble_update_conn_params(conn_handle, 20, 60, 0, 3000);

        } else {
            ESP_LOGI(TAG, "Error: Connection failed; status=%d\n",
                            event->connect.status);
            ble_advertise();
        }

        return;

    case BLE_GAP_EVENT_DISCONNECT:

        ESP_LOGI(TAG, "Disconnected >%d<", event->disconnect.reason);
        ble_advertise();
        return;

    case BLE_GAP_EVENT_CONN_UPDATE:

        ESP_LOGI(TAG, "Connection updated; status: [%d]",
                 event->conn_update.status);
        ble_gap_conn_find(event->conn_update.conn_handle, &desc);
        ble_print_conn_desc(&desc);                 
        return;

    case BLE_GAP_EVENT_ADV_COMPLETE:
        ESP_LOGI(TAG, "Advertise complete, reason=%d",
                    event->adv_complete.reason);
        ble_advertise();
        return;

    case BLE_GAP_EVENT_SUBSCRIBE:
        ESP_LOGI(TAG, "Sub event -> cur_notify: [%d] | val_handle: [0x%04x]",
                 event->subscribe.cur_notify, event->subscribe.attr_handle);
        if (event->subscribe.attr_handle == notify_handle) {
            notify_state = event->subscribe.cur_notify;
            xSemaphoreGive(notify_sem);
        } else {
            notify_state = event->subscribe.cur_notify;
        }
        return;

    case BLE_GAP_EVENT_NOTIFY_TX:
        ESP_LOGD(TAG, "BLE_GAP_EVENT_NOTIFY_TX success !!");
        if ((event->notify_tx.status == 0) ||
            (event->notify_tx.status == BLE_HS_EDONE)) {
            xSemaphoreGive(notify_sem);
        } else {
            ESP_LOGE(TAG, "BLE_GAP_EVENT_NOTIFY_TX notify tx status = %d", event->notify_tx.status);
        }
        return;

    case BLE_GAP_EVENT_MTU:
        ESP_LOGI(TAG, "MTU update event, conn_handle: [%d] | cid: [%d] | MTU: [%d]\n",
                    event->mtu.conn_handle,
                    event->mtu.channel_id,
                    event->mtu.value);
        return;

    default:
        ESP_LOGI(TAG, "Uncaught event: [%s]", ble_gap_event2str(event->type));
        return;
    } 
}

static void ble_notify_task(void *pv)
{
    const char* TAG = "BLE_NOTIFY_TASK";
    static char payload[50] = {0};/* Data payload */
    static int num = 0;
    struct os_mbuf *om;

    while (1) {

        if (!notify_state) {
            vTaskDelay(1000 / portTICK_PERIOD_MS);
            continue;
        }

        xSemaphoreTake(notify_sem, portMAX_DELAY);

        /* Check if the MBUFs are available */
        if (os_msys_num_free() >= 2) {
            do {
                memset(payload, 0, sizeof(payload));
                snprintf(payload, sizeof(payload), "Burn this city to the ground!: %d", num++);            
                om = ble_hs_mbuf_from_flat(payload, sizeof(payload));
                if (om == NULL) {
                    ESP_LOGE(TAG, "No MBUFs available from pool, retry..");
                    vTaskDelay(100 / portTICK_PERIOD_MS);
                }
            } while (om == NULL);

            BLE_FUNC_ERR_RET(ble_gatts_notify_custom(conn_handle, notify_handle, om),
                            "Error while sending notification");
            ESP_LOGI(TAG, "Notification sent to conn [%04x]: [%s]", conn_handle, payload);
        } else {
            ESP_LOGE(TAG, "Not enough OS_MBUFs available");
        }

        xSemaphoreGive(notify_sem);
        vTaskDelay(5000 / portTICK_PERIOD_MS);
    }
}



static void ble_svr_register_cb(struct ble_gatt_register_ctxt *ctxt, void *arg)
{
    const char *TAG = "BLE_SVR_REG_CB";
    char buf[BLE_UUID_STR_LEN];

    switch (ctxt->op) {
    case BLE_GATT_REGISTER_OP_SVC:
        printf("\n");
        ESP_LOGI(TAG, "Registered service %s with svc_handle: [%04x]",
                    ble_uuid_to_str(ctxt->svc.svc_def->uuid, buf),
                    ctxt->svc.handle);
        break;

    case BLE_GATT_REGISTER_OP_CHR:
        ESP_LOGI(TAG, "Registering characteristic %s with "
                    "def_handle: [%04x] | val_handle: [%04x]",
                    ble_uuid_to_str(ctxt->chr.chr_def->uuid, buf),
                    ctxt->chr.def_handle,
                    ctxt->chr.val_handle);
        break;

    case BLE_GATT_REGISTER_OP_DSC:
        ESP_LOGI(TAG, "Registering descriptor %s with dsc_handle: [%04x]",
                    ble_uuid_to_str(ctxt->dsc.dsc_def->uuid, buf),
                    ctxt->dsc.handle);
        break;

    default:
        ESP_LOGW(TAG, "unknown register operation");
        break;
    }
}


static void hs_reset_cb(int reason)
{
    ESP_LOGI("BLE_HOST_RESET", "Resetting state; reason=%d\n", reason);
}

static void hs_sync_cb(void)
{
    const char *TAG = "BLE_HOST_SYNC";
    /* Make sure we have proper identity address set (public preferred) */
    BLE_FUNC_ERR_RET(ble_hs_util_ensure_addr(0),
                        "ensure address failed");
    BLE_FUNC_ERR_RET(ble_hs_id_infer_auto(0, &own_addr_type),
                        "infer address type failed");

    uint16_t mtu = 0;
    uint16_t timeout = 0;

    // BLE_FUNC_ERR_RET(ble_gap_write_sugg_def_data_len(251, 1024),
    //                 "Failed to set suggested data length");  
    // BLE_FUNC_ERR_RET(ble_gap_read_sugg_def_data_len(&mtu, &timeout),
    //                 "Failed to set suggested data length");
    // ESP_LOGI(TAG, "MTU: [%d] | Timeout: [%d]", mtu, timeout);
    /* Printing ADDR */
    uint8_t addr_val[6] = {0};
    BLE_FUNC_ERR_RET(ble_hs_id_copy_addr(own_addr_type, addr_val, NULL),
                        "copy address failed");
    ESP_LOGI(TAG, "Device Address: ["MACSTR"], start advertising", MAC2STR(addr_val));

    ble_advertise();
}

void hs_task(void *pv)
{
    ESP_LOGI("BLE_HOST_TASK", "Task Started");
    notify_sem = xSemaphoreCreateCounting(100, 0);
    nimble_port_run();
    nimble_port_freertos_deinit();   
}

void ble_svr_init(void) {

    const char* TAG = "BLE_SVR_INIT";
    ESP_LOGI(TAG, "Initializing BLE Server");

    ble_svc_gap_init();
    ble_svc_gatt_init();

    BLE_FUNC_ERR_RET(ble_gatts_count_cfg(svr_defs),
                        "config adjusted failed");

    BLE_FUNC_ERR_RET(ble_gatts_add_svcs(svr_defs),
                        "add service failed");

    xTaskCreate(ble_notify_task, "notify_task", 4096, NULL, 10, NULL);
}

void app_main(void)
{
    const char* TAG = "APP_MAIN";
    ESP_LOGI(TAG, "Starting App");
    ESP_ERROR_CHECK(nvs_helper_init());

    ESP_ERROR_CHECK(nimble_port_init());

    /* Set the default device name. */
    ESP_ERROR_CHECK(ble_svc_gap_device_name_set("JOY-SERVER"));

    /* Register custom service */
    ble_svr_init();

    /* Configure the host. */
    ble_hs_cfg.reset_cb = hs_reset_cb;
    ble_hs_cfg.sync_cb  = hs_sync_cb;
    ble_hs_cfg.store_status_cb = ble_store_util_status_rr; 
    ble_hs_cfg.gatts_register_cb = ble_svr_register_cb;   
    ble_store_config_init();

    nimble_port_freertos_init(hs_task);
}


static int ble_update_conn_params(uint16_t conn_handle,
                            uint16_t minInterval, uint16_t maxInterval,
                            uint16_t latency, uint16_t timeout)
{
    const char* TAG = "BLE_UPDATE_CONN_PARAMS";
    struct ble_gap_upd_params params;

    params.latency  = latency;
    params.itvl_max = maxInterval;    // max_int = 0x20*1.25ms = 40ms
    params.itvl_min = minInterval;    // min_int = 0x10*1.25ms = 20ms
    params.supervision_timeout = timeout;    // timeout = 400*10ms = 4000ms
    params.min_ce_len = BLE_GAP_INITIAL_CONN_MIN_CE_LEN;  // Minimum length of connection event in 0.625ms units
    params.max_ce_len = BLE_GAP_INITIAL_CONN_MAX_CE_LEN;  // Maximum length of connection event in 0.625ms units

    int rc = ble_gap_update_params(conn_handle, &params);
    if (rc != 0) {
        ESP_LOGE(TAG, "Update params error: [%s]", ble_err2str(rc));
    } else {
        ESP_LOGI(TAG, "Update params success");
    }

    return rc;
} // updateConnParams


static const char* ble_err2str(int err_code) {
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

static const char* ble_gap_event2str(uint8_t event_type) {
    switch (event_type) {
        case BLE_GAP_EVENT_CONNECT:
            return "BLE_GAP_EVENT_CONNECT";
        case BLE_GAP_EVENT_DISCONNECT:
            return "BLE_GAP_EVENT_DISCONNECT";
        case BLE_GAP_EVENT_CONN_UPDATE:
            return "BLE_GAP_EVENT_CONN_UPDATE";
        case BLE_GAP_EVENT_CONN_UPDATE_REQ:
            return "BLE_GAP_EVENT_CONN_UPDATE_REQ";
        case BLE_GAP_EVENT_L2CAP_UPDATE_REQ:
            return "BLE_GAP_EVENT_L2CAP_UPDATE_REQ";
        case BLE_GAP_EVENT_TERM_FAILURE:
            return "BLE_GAP_EVENT_TERM_FAILURE";
        case BLE_GAP_EVENT_DISC:
            return "BLE_GAP_EVENT_DISC";
        case BLE_GAP_EVENT_DISC_COMPLETE:
            return "BLE_GAP_EVENT_DISC_COMPLETE";
        case BLE_GAP_EVENT_ADV_COMPLETE:
            return "BLE_GAP_EVENT_ADV_COMPLETE";
        case BLE_GAP_EVENT_ENC_CHANGE:
            return "BLE_GAP_EVENT_ENC_CHANGE";
        case BLE_GAP_EVENT_PASSKEY_ACTION:
            return "BLE_GAP_EVENT_PASSKEY_ACTION";
        case BLE_GAP_EVENT_NOTIFY_RX:
            return "BLE_GAP_EVENT_NOTIFY_RX";
        case BLE_GAP_EVENT_NOTIFY_TX:
            return "BLE_GAP_EVENT_NOTIFY_TX";
        case BLE_GAP_EVENT_SUBSCRIBE:
            return "BLE_GAP_EVENT_SUBSCRIBE";
        case BLE_GAP_EVENT_MTU:
            return "BLE_GAP_EVENT_MTU";
        case BLE_GAP_EVENT_IDENTITY_RESOLVED:
            return "BLE_GAP_EVENT_IDENTITY_RESOLVED";
        case BLE_GAP_EVENT_REPEAT_PAIRING:
            return "BLE_GAP_EVENT_REPEAT_PAIRING";
        case BLE_GAP_EVENT_PHY_UPDATE_COMPLETE:
            return "BLE_GAP_EVENT_PHY_UPDATE_COMPLETE";
        case BLE_GAP_EVENT_EXT_DISC:
            return "BLE_GAP_EVENT_EXT_DISC";
        case BLE_GAP_EVENT_PERIODIC_SYNC:
            return "BLE_GAP_EVENT_PERIODIC_SYNC";
        case BLE_GAP_EVENT_PERIODIC_REPORT:
            return "BLE_GAP_EVENT_PERIODIC_REPORT";
        case BLE_GAP_EVENT_PERIODIC_SYNC_LOST:
            return "BLE_GAP_EVENT_PERIODIC_SYNC_LOST";
        case BLE_GAP_EVENT_SCAN_REQ_RCVD:
            return "BLE_GAP_EVENT_SCAN_REQ_RCVD";
        case BLE_GAP_EVENT_PERIODIC_TRANSFER:
            return "BLE_GAP_EVENT_PERIODIC_TRANSFER";
        case BLE_GAP_EVENT_PATHLOSS_THRESHOLD:
            return "BLE_GAP_EVENT_PATHLOSS_THRESHOLD";
        case BLE_GAP_EVENT_TRANSMIT_POWER:
            return "BLE_GAP_EVENT_TRANSMIT_POWER";
        case BLE_GAP_EVENT_SUBRATE_CHANGE:
            return "BLE_GAP_EVENT_SUBRATE_CHANGE";
        case BLE_GAP_EVENT_VS_HCI:
            return "BLE_GAP_EVENT_VS_HCI";
        case BLE_GAP_EVENT_REATTEMPT_COUNT:
            return "BLE_GAP_EVENT_REATTEMPT_COUNT";
        case BLE_GAP_EVENT_AUTHORIZE:
            return "BLE_GAP_EVENT_AUTHORIZE";
        case BLE_GAP_EVENT_TEST_UPDATE:
            return "BLE_GAP_EVENT_TEST_UPDATE";
        default:
            return "Unknown BLE_GAP_EVENT";
    }
}

