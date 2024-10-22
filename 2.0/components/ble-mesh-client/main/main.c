#include <stdio.h>
#include <string.h>
#include <inttypes.h>

#include "esp_log.h"
#include "esp_mac.h"
#include "nvs_flash.h"
#include "esp_timer.h"

#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "host/ble_hs.h"
#include "host/util/util.h"
#include "store/config/ble_store_config.h"

#include "esp_ble_mesh_defs.h"
#include "esp_ble_mesh_common_api.h"
#include "esp_ble_mesh_provisioning_api.h"
#include "esp_ble_mesh_networking_api.h"
#include "esp_ble_mesh_config_model_api.h"
#include "esp_ble_mesh_generic_model_api.h"

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

#include "helpers/nvs_helper.c"
#include "helpers/mem_helper.c"
#include "my_utils.h"

#define CID_ESP             0x02E5

#define PROV_OWN_ADDR       0x0001

#define MSG_SEND_TTL        3
#define MSG_TIMEOUT         0
#define MSG_ROLE            ROLE_PROVISIONER

#define COMP_DATA_PAGE_0    0x00

#define NET_KEY_IDX         0x0000
#define APP_KEY_IDX         0x0000
#define NET_KEY             {0xe4, 0x91, 0x9f, 0xbd, 0x8b, 0x05, 0x21, 0xfe, 0x7e, 0x9c, 0x7a, 0x68, 0xca, 0x06, 0x72, 0x8f}
#define APP_KEY             0x12

#define BLE_MESH_MAX_MSG_LEN                500

#define COMP_DATA_1_OCTET(msg, offset)      (msg[offset])
#define COMP_DATA_2_OCTET(msg, offset)      (msg[offset + 1] << 8 | msg[offset])

#define ESP_BLE_MESH_VND_MODEL_ID_CLIENT    0x0000
#define ESP_BLE_MESH_VND_MODEL_ID_SERVER    0x0001

#define ESP_BLE_MESH_VND_MODEL_OP_SEND      ESP_BLE_MESH_MODEL_OP_3(0x00, CID_ESP)
#define ESP_BLE_MESH_VND_MODEL_OP_ACK       ESP_BLE_MESH_MODEL_OP_3(0x01, CID_ESP)

#define UUIDSTR   "%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x%02x%02x"
#define UUID2STR(u) u[0], u[1], u[2], u[3], u[4], u[5], u[6], u[7], \
                    u[8], u[9], u[10], u[11], u[12], u[13], u[14], u[15]

static SemaphoreHandle_t mesh_sem;
static uint8_t own_addr_type;
static uint8_t addr_val[6] = {0};

static bool write_enable = false;
static bool node_enabled = false;

static const char * NVS_NS  = "mesh_vendor";
static const char * NVS_KEY = "vendor_client";

static uint8_t dev_uuid[16];

typedef struct {
    uint16_t len;
    char msg[0];  
} my_msg_t;

typedef struct {
    uint16_t server_addr;   /* Vendor server unicast address */
    uint16_t vnd_tid;       /* TID contained in the vendor message */
    uint8_t  onoff;         /* Remote OnOff */
} node_info_t;

static node_info_t node_info = {
    .server_addr = 0x0000,
    .vnd_tid     = 0,
    .onoff      = 0,
};

static struct esp_ble_mesh_key {
    uint16_t net_idx;
    uint16_t app_idx;
    uint8_t  net_key[16];
    uint8_t  app_key[16];
} prov_key;

static esp_ble_mesh_cfg_srv_t config_server = {
    /* 3 transmissions with 20ms interval */
    .net_transmit = ESP_BLE_MESH_TRANSMIT(1, 20),
    .relay = ESP_BLE_MESH_RELAY_DISABLED,
    .relay_retransmit = ESP_BLE_MESH_TRANSMIT(1, 20),
    .gatt_proxy = ESP_BLE_MESH_GATT_PROXY_DISABLED,
    .beacon = ESP_BLE_MESH_BEACON_DISABLED,
    .friend_state = ESP_BLE_MESH_FRIEND_NOT_SUPPORTED,
    .default_ttl = 7,
};

static esp_ble_mesh_client_t config_client;

static esp_ble_mesh_client_t onoff_client;
ESP_BLE_MESH_MODEL_PUB_DEFINE(onoff_cli_pub, 2 + 1, ROLE_NODE);

static const esp_ble_mesh_client_op_pair_t vnd_op_pair[] = {
    {ESP_BLE_MESH_VND_MODEL_OP_SEND, ESP_BLE_MESH_VND_MODEL_OP_ACK},
};

static esp_ble_mesh_client_t vendor_client = {
    .op_pair_size = ARRAY_SIZE(vnd_op_pair),
    .op_pair = vnd_op_pair,
};

static esp_ble_mesh_model_op_t vnd_op[] = {
    // {ESP_BLE_MESH_VND_MODEL_OP_SEND,   1, (uint32_t)NULL},
    ESP_BLE_MESH_MODEL_OP(ESP_BLE_MESH_VND_MODEL_OP_SEND, 2),
    ESP_BLE_MESH_MODEL_OP(ESP_BLE_MESH_VND_MODEL_OP_ACK,  1),
    ESP_BLE_MESH_MODEL_OP_END,
};

static esp_ble_mesh_model_t vnd_models[] = {
    ESP_BLE_MESH_VENDOR_MODEL(CID_ESP, ESP_BLE_MESH_VND_MODEL_ID_CLIENT,
                              vnd_op, NULL, &vendor_client),                            
};

static esp_ble_mesh_model_t root_models[] = {
    ESP_BLE_MESH_MODEL_CFG_SRV(&config_server),
    ESP_BLE_MESH_MODEL_CFG_CLI(&config_client),
    ESP_BLE_MESH_MODEL_GEN_ONOFF_CLI(&onoff_cli_pub, &onoff_client),
};

static esp_ble_mesh_elem_t elements[] = {
    ESP_BLE_MESH_ELEMENT(0, root_models, vnd_models),
};

static esp_ble_mesh_comp_t composition = {
    .cid = CID_ESP,
    .element_count = ARRAY_SIZE(elements),
    .elements = elements,
};

static esp_ble_mesh_prov_t provision = {
    .prov_uuid          = dev_uuid,
    .prov_unicast_addr  = PROV_OWN_ADDR,
    .prov_start_address = 0x0005,
};

void ble_store_config_init(void);
extern void touch_myinit(void);

#define BLE_FUNC_ERR_RET(con, format, ...) do { \
    int ret = (con); \
        if ((ret) != 0) { \
            if(*format != '\0') \
                ESP_LOGW(TAG, "[%s, %d] <%s> " format, __func__, __LINE__, ble_err2str(ret), ##__VA_ARGS__); \
            return; \
        } \
    } while(0) 


static void ble_node_info_store(void)
{
    const char *TAG = "BLE_MESH_INFO_STORE";
    ESP_LOG_INFO(nvs_helper_blob_set(NVS_NS, NVS_KEY, &node_info, sizeof(node_info)),
                 "Storing info failed");
    ESP_LOGI(TAG, "Stored info successfully");
}

static void ble_node_info_restore(void)
{
    const char *TAG = "BLE_MESH_INFO_RESTORE";

    ESP_LOG_INFO(nvs_helper_blob_get(NVS_NS, NVS_KEY, &node_info, sizeof(node_info)),
                 "Restoring info failed");
    ESP_LOGI(TAG, "Restored info successfully");
}

static void ble_mesh_set_msg_common(esp_ble_mesh_client_common_param_t *common,
                                            esp_ble_mesh_node_t *node,
                                            esp_ble_mesh_model_t *model, uint32_t opcode)
{
    common->opcode = opcode;
    common->model = model;
    common->ctx.net_idx = prov_key.net_idx;
    common->ctx.app_idx = prov_key.app_idx;
    common->ctx.addr = node->unicast_addr;
    common->ctx.send_ttl = MSG_SEND_TTL;
    common->msg_timeout = MSG_TIMEOUT;
}

static esp_err_t ble_mesh_node_init(uint16_t node_addr)
{
    const char *TAG = "BLE_MESH_NODE_INIT";
    esp_ble_mesh_client_common_param_t  common = {0};
    esp_ble_mesh_cfg_client_get_state_t get    = {0};    
    esp_ble_mesh_node_t *node = NULL;

    node = esp_ble_mesh_provisioner_get_node_with_addr(node_addr);
    ESP_ERROR_RETURN(node == NULL, ESP_FAIL, "Failed to get node 0x%04x info", node_addr);


    ble_mesh_set_msg_common(&common, node, config_client.model, ESP_BLE_MESH_MODEL_OP_COMPOSITION_DATA_GET);
    get.comp_data_get.page = COMP_DATA_PAGE_0;
    ESP_ERR_FUNC_RET(esp_ble_mesh_config_client_get_state(&common, &get),
                     "Failed to send Config Composition Data Get");

    return ESP_OK;
}

static esp_err_t ble_mesh_prov_complete(uint16_t node_index, const esp_ble_mesh_octet16_t uuid,
                                  uint16_t primary_addr, uint8_t element_num, uint16_t net_idx)
{
    const char *TAG = "BLE_MESH_PROV_COMPLETE";

    esp_ble_mesh_client_common_param_t  common = {0};
    esp_ble_mesh_cfg_client_get_state_t get    = {0};
    esp_ble_mesh_node_t *node = NULL;
    char name[10] = {0};
    esp_err_t ret;

    ESP_LOGI(TAG, "Device UUID ["UUIDSTR"] | node_idx %u | primary_addr 0x%04x | elem_num %u | net_idx 0x%03x",
             UUID2STR(uuid), node_index, primary_addr, element_num, net_idx);

    node_info.server_addr = primary_addr;
    ble_node_info_store(); /* Store proper mesh example info */

    sprintf(name, "%s%02x", "NODE-", node_index);
    ESP_ERR_FUNC_RET(esp_ble_mesh_provisioner_set_node_name(node_index, name),
                     "Failed to set node name");

    node = esp_ble_mesh_provisioner_get_node_with_addr(primary_addr);
    ESP_ERROR_RETURN(node == NULL, ESP_FAIL, "Failed to get node 0x%04x info", primary_addr);


    ble_mesh_set_msg_common(&common, node, config_client.model, ESP_BLE_MESH_MODEL_OP_COMPOSITION_DATA_GET);
    get.comp_data_get.page = COMP_DATA_PAGE_0;
    ESP_ERR_FUNC_RET(esp_ble_mesh_config_client_get_state(&common, &get),
                     "Failed to send Config Composition Data Get");

    return ESP_OK;
}

static esp_err_t ble_mesh_recv_unprov_adv_pkt(uint8_t dev_uuid[16], uint8_t addr[BD_ADDR_LEN],
                                    esp_ble_mesh_addr_type_t addr_type, uint16_t oob_info,
                                    uint8_t adv_type, esp_ble_mesh_prov_bearer_t bearer)
{
    esp_ble_mesh_unprov_dev_add_t add_dev = {0};
    esp_err_t ret = ESP_OK;

    ESP_LOGI(TAG, "Device MAC ["MACSTR"] | type: [0x%02x] | adv: [0x%02x]", 
             MAC2STR(addr), addr_type, adv_type);
    ESP_LOGI(TAG, "Device UUID ["UUIDSTR"] | oob: [0x%04x] | bearer: [%s]", 
             UUID2STR(dev_uuid), oob_info, (bearer & ESP_BLE_MESH_PROV_ADV) ? "PB-ADV" : "PB-GATT");

    memcpy(add_dev.addr,       addr, BD_ADDR_LEN);
           add_dev.addr_type = (esp_ble_mesh_addr_type_t)addr_type;
    memcpy(add_dev.uuid,       dev_uuid, 16);
           add_dev.oob_info  = oob_info;
           add_dev.bearer    = (esp_ble_mesh_prov_bearer_t)bearer;

    ESP_ERR_FUNC_RET(esp_ble_mesh_provisioner_add_unprov_dev(&add_dev,
                ADD_DEV_RM_AFTER_PROV_FLAG | ADD_DEV_START_PROV_NOW_FLAG | ADD_DEV_FLUSHABLE_DEV_FLAG),
                "Failed to add unprovisioned device");

    return ESP_OK;
}

static void ble_mesh_provisioning_cb(esp_ble_mesh_prov_cb_event_t event,
                                             esp_ble_mesh_prov_cb_param_t *param)
{
    const char *TAG = "BLE_MESH_PROV_CB";
    esp_err_t ret = ESP_OK;

    switch (event) {
    case ESP_BLE_MESH_PROV_REGISTER_COMP_EVT:
        ESP_LOGI(TAG, "PROV_REGISTER_COMP | Err: >%d<", 
            param->prov_register_comp.err_code);
        ble_node_info_restore();
        break;

    case ESP_BLE_MESH_PROVISIONER_PROV_ENABLE_COMP_EVT:
        ESP_LOGI(TAG, "PROV_ENABLE_COMP | Err: >%d<", 
            param->provisioner_prov_enable_comp.err_code);
        break;

    case ESP_BLE_MESH_PROVISIONER_PROV_DISABLE_COMP_EVT:
        ESP_LOGI(TAG, "PROV_DISABLE_COMP | Err: >%d<", 
            param->provisioner_prov_disable_comp.err_code);
        break;

    case ESP_BLE_MESH_PROVISIONER_RECV_UNPROV_ADV_PKT_EVT:
        ESP_LOGI(TAG, "RECV_UNPROV_ADV_PKT_EVT");
        ble_mesh_recv_unprov_adv_pkt(
            param->provisioner_recv_unprov_adv_pkt.dev_uuid, 
            param->provisioner_recv_unprov_adv_pkt.addr,
            param->provisioner_recv_unprov_adv_pkt.addr_type,
            param->provisioner_recv_unprov_adv_pkt.oob_info,
            param->provisioner_recv_unprov_adv_pkt.adv_type,
            param->provisioner_recv_unprov_adv_pkt.bearer);
        break;

    case ESP_BLE_MESH_PROVISIONER_PROV_LINK_OPEN_EVT:
        ESP_LOGI(TAG, "PROV_LINK_OPEN | Bearer: [%s]",
            param->provisioner_prov_link_open.bearer == 1 ? "PB-ADV" : "PB-GATT");
        break;

    case ESP_BLE_MESH_PROVISIONER_PROV_LINK_CLOSE_EVT:
        ESP_LOGI(TAG, "PROV_LINK_CLOSE | Bearer: [%s] | Reason: [0x%02x]",
            param->provisioner_prov_link_close.bearer == 1 ? "PB-ADV" : "PB-GATT", 
            param->provisioner_prov_link_close.reason);
        break;

    case ESP_BLE_MESH_PROVISIONER_PROV_COMPLETE_EVT:
        ble_mesh_prov_complete(
            param->provisioner_prov_complete.node_idx,     
            param->provisioner_prov_complete.device_uuid,
            param->provisioner_prov_complete.unicast_addr, 
            param->provisioner_prov_complete.element_num,
            param->provisioner_prov_complete.netkey_idx);
        break;

    case ESP_BLE_MESH_PROVISIONER_ADD_UNPROV_DEV_COMP_EVT:
        ESP_LOGI(TAG, "ADD_UNPROV_DEV_COMP | Err: >%d<", 
            param->provisioner_add_unprov_dev_comp.err_code);
        break;

    case ESP_BLE_MESH_PROVISIONER_SET_DEV_UUID_MATCH_COMP_EVT:
        ESP_LOGI(TAG, "SET_DEV_UUID_MATCH_COMP | Err: >%d<", 
            param->provisioner_set_dev_uuid_match_comp.err_code);
        break;

    case ESP_BLE_MESH_PROVISIONER_SET_NODE_NAME_COMP_EVT:
        uint8_t code = param->provisioner_set_node_name_comp.err_code;
        uint16_t idx = param->provisioner_set_node_name_comp.node_index;
        ESP_LOGI(TAG, "SET_NODE_NAME_COMP | Err: >%d<", code);
        if (code == 0) {
            char *name = esp_ble_mesh_provisioner_get_node_name(idx);
            if (name)  ESP_LOGI(TAG, "Node [%d] Name: [%s]", idx, name);}
        break;

    case ESP_BLE_MESH_PROVISIONER_ADD_LOCAL_APP_KEY_COMP_EVT:
        uint8_t code1 = param->provisioner_add_app_key_comp.err_code;
        ESP_LOGI(TAG, "ADD_LOCAL_APP_KEY_COMP | Err: >%d<", code1);
        if (code1 == 0) {
            prov_key.app_idx = param->provisioner_add_app_key_comp.app_idx;
            ESP_LOG_INFO(esp_ble_mesh_provisioner_bind_app_key_to_local_model(
                        PROV_OWN_ADDR, prov_key.app_idx, 
                        ESP_BLE_MESH_VND_MODEL_ID_CLIENT, CID_ESP),
                        "Failed to bind AppKey to vendor client");
            ESP_LOG_INFO(esp_ble_mesh_provisioner_bind_app_key_to_local_model(
                        PROV_OWN_ADDR, prov_key.app_idx, 
                        ESP_BLE_MESH_MODEL_ID_GEN_ONOFF_CLI, ESP_BLE_MESH_CID_NVAL),
                        "Failed to bind AppKey to on/off client");}                        
        break;

    case ESP_BLE_MESH_PROVISIONER_BIND_APP_KEY_TO_MODEL_COMP_EVT:
        ESP_LOGI(TAG, "BIND_APP_KEY_TO_MODEL_COMP | Err: >%d<", 
            param->provisioner_bind_app_key_to_model_comp.err_code);
        break;

    case ESP_BLE_MESH_PROVISIONER_STORE_NODE_COMP_DATA_COMP_EVT:
        ESP_LOGI(TAG, "STORE_NODE_COMP_DATA_COMP | Err: >%d<", 
            param->provisioner_store_node_comp_data_comp.err_code);
        break;
    default:
        break;
    }

    return;
}

void ble_mesh_send_gen_onoff_set(void)
{
    const char *TAG = "BLE_MESH_CLI_ONOFF";

    esp_ble_mesh_generic_client_set_state_t set = {0};
    esp_ble_mesh_client_common_param_t common = {0};
    esp_err_t err = ESP_OK;

    common.opcode = ESP_BLE_MESH_MODEL_OP_GEN_ONOFF_SET_UNACK;
    common.model = onoff_client.model;
    common.ctx.net_idx = prov_key.net_idx;
    common.ctx.app_idx = prov_key.app_idx;
    common.ctx.addr = 0x0005;   /* to all nodes */
    common.ctx.send_ttl = 7;
    common.msg_timeout = 0;     /* 0 indicates that timeout value from menuconfig will be used */


    set.onoff_set.op_en = false;
    set.onoff_set.onoff = node_info.onoff;
    set.onoff_set.tid   = node_info.vnd_tid++;

    ESP_LOG_INFO(esp_ble_mesh_generic_client_set_state(&common, &set),
                     "Failed to send Generic OnOff Set Unack");

    node_info.onoff = !node_info.onoff;

    ESP_LOGI(TAG, "Send Generic OnOff Status: [%d]", node_info.onoff);

}

static void ble_mesh_generic_client_cb(esp_ble_mesh_generic_client_cb_event_t event,
                                       esp_ble_mesh_generic_client_cb_param_t *param)
{
    const char *TAG = "BLE_MESH_GEN_CLI_CB";
    ESP_LOGI(TAG, "Generic client, event %u, error code %d, opcode is 0x%04" PRIx32,
        event, param->error_code, param->params->opcode);

    switch (event) {
    case ESP_BLE_MESH_GENERIC_CLIENT_GET_STATE_EVT:
        ESP_LOGI(TAG, "GET_STATE");
        if (param->params->opcode == ESP_BLE_MESH_MODEL_OP_GEN_ONOFF_GET) {
            ESP_LOGI(TAG, "GEN_ONOFF_GET, onoff: [%d]", param->status_cb.onoff_status.present_onoff);
        }
        break;
    case ESP_BLE_MESH_GENERIC_CLIENT_SET_STATE_EVT:
        ESP_LOGI(TAG, "SET_STATE");
        if (param->params->opcode == ESP_BLE_MESH_MODEL_OP_GEN_ONOFF_SET) {
            ESP_LOGI(TAG, "GEN_ONOFF_SET, onoff: [%d]", param->status_cb.onoff_status.present_onoff);
        }
        break;
    case ESP_BLE_MESH_GENERIC_CLIENT_PUBLISH_EVT:
        ESP_LOGI(TAG, "PUBLISH");
        break;
    case ESP_BLE_MESH_GENERIC_CLIENT_TIMEOUT_EVT:
        ESP_LOGI(TAG, "TIMEOUT");
        if (param->params->opcode == ESP_BLE_MESH_MODEL_OP_GEN_ONOFF_SET) {
            /* If failed to get the response of Generic OnOff Set, resend Generic OnOff Set  */
            ble_mesh_send_gen_onoff_set();
        }
        break;
    default:
        break;
    }
}


static void ble_mesh_parse_node_comp_data(const uint8_t *data, uint16_t length)
{
    uint16_t cid, pid, vid, crpl, feat;
    uint16_t loc, model_id, company_id;
    uint8_t nums, numv;
    uint16_t offset;
    int i;

    cid = COMP_DATA_2_OCTET(data, 0);
    pid = COMP_DATA_2_OCTET(data, 2);
    vid = COMP_DATA_2_OCTET(data, 4);
    crpl = COMP_DATA_2_OCTET(data, 6);
    feat = COMP_DATA_2_OCTET(data, 8);
    offset = 10;

    // ESP_LOGI(TAG, "********************** Composition Data Start **********************");
    // ESP_LOGI(TAG, "* CID 0x%04x, PID 0x%04x, VID 0x%04x, CRPL 0x%04x, Features 0x%04x *", cid, pid, vid, crpl, feat);
    while (offset < length) {
        loc = COMP_DATA_2_OCTET(data, offset);
        nums = COMP_DATA_1_OCTET(data, offset + 2);
        numv = COMP_DATA_1_OCTET(data, offset + 3);
        offset += 4;
        // ESP_LOGI(TAG, "* Loc 0x%04x, NumS 0x%02x, NumV 0x%02x *", loc, nums, numv);
        for (i = 0; i < nums; i++) {
            model_id = COMP_DATA_2_OCTET(data, offset);
            // ESP_LOGI(TAG, "* SIG Model ID 0x%04x *", model_id);
            offset += 2;
        }
        for (i = 0; i < numv; i++) {
            company_id = COMP_DATA_2_OCTET(data, offset);
            model_id = COMP_DATA_2_OCTET(data, offset + 2);
            // ESP_LOGI(TAG, "* Vendor Model ID 0x%04x, Company ID 0x%04x *", model_id, company_id);
            offset += 4;
        }
    }
    // ESP_LOGI(TAG, "*********************** Composition Data End ***********************");
}

static void ble_mesh_config_client_cb(esp_ble_mesh_cfg_client_cb_event_t event,
                                              esp_ble_mesh_cfg_client_cb_param_t *param)
{
    const char *TAG = "BLE_MESH_CFG_CLI_CB";

    esp_ble_mesh_client_common_param_t common = {0};
    esp_ble_mesh_cfg_client_set_state_t set = {0};
    esp_ble_mesh_node_t *node = NULL;
    esp_err_t ret = ESP_OK;

    ESP_LOGI(TAG, "Err: [%d] | Event: [%u] | Addr: [0x%04x] | Opcode: [0x%04lu]",
        param->error_code, event, param->params->ctx.addr, param->params->opcode);
    if (param->error_code) {
        ESP_LOGE(TAG, "Send config client message failed, opcode 0x%04lu", param->params->opcode);
        return;
    }

    node = esp_ble_mesh_provisioner_get_node_with_addr(param->params->ctx.addr);
    if(node == NULL) {
        ESP_LOGE(TAG, "Failed to get node 0x%04x info", param->params->ctx.addr);
        return;
    }

    switch (event) {
    case ESP_BLE_MESH_CFG_CLIENT_GET_STATE_EVT:
        if (param->params->opcode == ESP_BLE_MESH_MODEL_OP_COMPOSITION_DATA_GET) {
            ESP_LOGI(TAG, "COMPOSITION_DATA_GET, sending add APP Key request...");
            ble_mesh_parse_node_comp_data(
                param->status_cb.comp_data_status.composition_data->data,
                param->status_cb.comp_data_status.composition_data->len);

            ESP_LOG_INFO(esp_ble_mesh_provisioner_store_node_comp_data(
                param->params->ctx.addr,
                param->status_cb.comp_data_status.composition_data->data,
                param->status_cb.comp_data_status.composition_data->len),
                "Failed to store node composition data");

            ble_mesh_set_msg_common(&common, node, config_client.model, 
                                    ESP_BLE_MESH_MODEL_OP_APP_KEY_ADD);
                   set.app_key_add.net_idx = prov_key.net_idx;
                   set.app_key_add.app_idx = prov_key.app_idx;
            memcpy(set.app_key_add.app_key,  prov_key.app_key, 16);

            ESP_LOG_INFO(esp_ble_mesh_config_client_set_state(&common, &set),
                             "Failed to send Config AppKey Add");
        }
        break;

    case ESP_BLE_MESH_CFG_CLIENT_SET_STATE_EVT:
        if (param->params->opcode == ESP_BLE_MESH_MODEL_OP_APP_KEY_ADD) {
            ESP_LOGI(TAG, "APP_KEY_ADD, sending bind model app request...");
            ble_mesh_set_msg_common(&common, node, config_client.model, 
                                    ESP_BLE_MESH_MODEL_OP_MODEL_APP_BIND);
            set.model_app_bind.element_addr  = node->unicast_addr;
            set.model_app_bind.model_app_idx = prov_key.app_idx;
            set.model_app_bind.model_id      = ESP_BLE_MESH_VND_MODEL_ID_SERVER;
            set.model_app_bind.company_id    = CID_ESP;
            ESP_LOG_INFO(esp_ble_mesh_config_client_set_state(&common, &set),
                             "Failed to send Config Model App Bind");
        } 
        else if (param->params->opcode == ESP_BLE_MESH_MODEL_OP_MODEL_APP_BIND) {
            ESP_LOGI(TAG, "MODEL_APP_BIND finished, getting onoff status");
            esp_ble_mesh_generic_client_get_state_t get_state = {0};
            ble_mesh_set_msg_common(&common, node, onoff_client.model, ESP_BLE_MESH_MODEL_OP_GEN_ONOFF_GET);
            ESP_LOG_INFO(esp_ble_mesh_generic_client_get_state(&common, &get_state),
                             "Failed to send Generic OnOff Get");
            // node_enabled = true;
            // write_enable = true;
        }
        break;

    case ESP_BLE_MESH_CFG_CLIENT_PUBLISH_EVT:
        if (param->params->opcode == ESP_BLE_MESH_MODEL_OP_COMPOSITION_DATA_STATUS) {
            ESP_LOGI(TAG, "COMPOSITION_DATA_STATUS, omitted");
        }
        break;

    case ESP_BLE_MESH_CFG_CLIENT_TIMEOUT_EVT:
        switch (param->params->opcode) {
        case ESP_BLE_MESH_MODEL_OP_COMPOSITION_DATA_GET: {
            ESP_LOGI(TAG, "COMPOSITION_DATA_GET Timeout, retrying...");

            esp_ble_mesh_cfg_client_get_state_t get = {0};
            ble_mesh_set_msg_common(&common, node, config_client.model, 
                ESP_BLE_MESH_MODEL_OP_COMPOSITION_DATA_GET);
            get.comp_data_get.page = COMP_DATA_PAGE_0;

            ESP_LOG_INFO(esp_ble_mesh_config_client_get_state(&common, &get),
                             "Failed to send Config Composition Data Get");
            break;
        }
        case ESP_BLE_MESH_MODEL_OP_APP_KEY_ADD:
            ESP_LOGI(TAG, "APP_KEY_ADD Timeout, retrying...");

            ble_mesh_set_msg_common(&common, node, config_client.model, 
                ESP_BLE_MESH_MODEL_OP_APP_KEY_ADD);
            set.app_key_add.net_idx = prov_key.net_idx;
            set.app_key_add.app_idx = prov_key.app_idx;
            memcpy(set.app_key_add.app_key, prov_key.app_key, 16);

            ESP_LOG_INFO(esp_ble_mesh_config_client_set_state(&common, &set),
                             "Failed to send Config AppKey Add");
            break;
        case ESP_BLE_MESH_MODEL_OP_MODEL_APP_BIND:
            ESP_LOGI(TAG, "MODEL_APP_BIND Timeout, retrying...");

            ble_mesh_set_msg_common(&common, node, config_client.model, 
                ESP_BLE_MESH_MODEL_OP_MODEL_APP_BIND);
            set.model_app_bind.element_addr  = node->unicast_addr;
            set.model_app_bind.model_app_idx = prov_key.app_idx;
            set.model_app_bind.model_id      = ESP_BLE_MESH_VND_MODEL_ID_SERVER;
            set.model_app_bind.company_id    = CID_ESP;

            ESP_LOG_INFO(esp_ble_mesh_config_client_set_state(&common, &set),
                             "Failed to send Config Model App Bind");
            break;
        default:
            break;
        }
        break;
    default:
        ESP_LOGI(TAG, "Invalid config client event %u", event);
        break;
    }

    return;
}


esp_err_t ble_mesh_send_vendor_message(uint16_t dst_addr, char* msg, uint16_t len)
{
    const char *TAG = "BLE_MESH_SEND_MSG";
    esp_ble_mesh_msg_ctx_t ctx = {0};
    uint32_t opcode;
    esp_err_t ret = ESP_OK;

    ctx.net_idx  = prov_key.net_idx;
    ctx.app_idx  = prov_key.app_idx;
    ctx.addr     = dst_addr;
    ctx.send_ttl = MSG_SEND_TTL;
    opcode       = ESP_BLE_MESH_VND_MODEL_OP_SEND;

    len += 1;
    char *my_msg = (char *)calloc(len, sizeof(char));
    if (len > BLE_MESH_MAX_MSG_LEN) {
        ESP_LOGE(TAG, "Message length is too long");
        return ESP_ERR_INVALID_ARG;
    }
    memcpy(my_msg, msg, len);

    ESP_ERROR_GOTO(esp_ble_mesh_client_model_send_msg(vendor_client.model, &ctx, opcode,
                     len, (uint8_t *)my_msg, MSG_TIMEOUT, true, MSG_ROLE),
                     EXIT, "Failed to send vendor message");

    ESP_LOGI(TAG, "Send message: [%s] len [%d] >>> Srv: [0x%04x]", my_msg, len, ctx.addr);

EXIT:
    free(my_msg);
    return ret;
}

void ble_send_task(void *pv) {

    const char* TAG = "BLE_WRITE_TASK";
    static char my_msg[50];
    static int num = 0;

    while (1) {

        if (!write_enable) {
            vTaskDelay(1000 / portTICK_PERIOD_MS);
            continue;
        }

        memset(my_msg, 0, sizeof(my_msg));
        snprintf(my_msg, sizeof(my_msg), "he:%d", num++);
        // ble_mesh_send_vendor_message(node_info.server_addr, my_msg, sizeof(my_msg));
        vTaskDelay(pdMS_TO_TICKS(5000)); // Wait for 5 seconds
    }
}


static void ble_mesh_my_model_cb(esp_ble_mesh_model_cb_event_t event,
                                             esp_ble_mesh_model_cb_param_t *param)
{
    const char *TAG = "BLE_MESH_MY_MODEL_CB";
    esp_err_t ret = ESP_OK;

    switch (event) {
    case ESP_BLE_MESH_MODEL_OPERATION_EVT:
        if (param->model_operation.opcode == ESP_BLE_MESH_VND_MODEL_OP_ACK) {
            ESP_LOGI(TAG, "Src: [0x%04x] | Dst: [0x%04x] | Recv Ack msg : [%s]",
                    param->model_operation.ctx->addr,
                    param->model_operation.ctx->recv_dst,
                    (char *)param->model_operation.msg);
        }
        else if (param->model_operation.opcode == ESP_BLE_MESH_VND_MODEL_OP_SEND) {
            ESP_LOGI(TAG, "Src: [0x%04x] | Dst: [0x%04x] | Recv Server msg : [%s]",
                    param->model_operation.ctx->addr,
                    param->model_operation.ctx->recv_dst,
                    (char *)param->model_operation.msg);
        }        
        break;

    case ESP_BLE_MESH_MODEL_SEND_COMP_EVT:
        if (param->model_send_comp.err_code) {
            ESP_LOGE(TAG, "Failed to send message 0x%06lu", param->model_send_comp.opcode);
            break;
        }
        // ESP_LOGI(TAG, "Send 0x%06lu", param->model_send_comp.opcode);
        break;

    case ESP_BLE_MESH_CLIENT_MODEL_RECV_PUBLISH_MSG_EVT:
        ESP_LOGI(TAG, "RECV_PUBLISH_MSG");
        if (param->client_recv_publish_msg.opcode == ESP_BLE_MESH_VND_MODEL_OP_SEND)
        ESP_LOGI(TAG, "Src: [0x%04x] | Dst: [0x%04x] | Recv Server msg : [%s]",
                param->client_recv_publish_msg.ctx->addr,
                param->client_recv_publish_msg.ctx->recv_dst,
                (char *)param->client_recv_publish_msg.msg);
        break;

    case ESP_BLE_MESH_CLIENT_MODEL_SEND_TIMEOUT_EVT:
        ESP_LOGW(TAG, "Client message 0x%06lu timeout", param->client_send_timeout.opcode);
        // ble_mesh_send_vendor_message("TimeOut", 8, true);
        break;
    default:
        break;
    }

    return;
}


void ble_mesh_get_dev_uuid(uint8_t *dev_uuid)
{
    if (dev_uuid == NULL) {
        ESP_LOGE("BLE_MESH_DEV_UUID", "%s, Invalid device uuid", __func__);
        return;
    }
    memcpy(dev_uuid + 2, addr_val, BD_ADDR_LEN);
    ESP_LOGI("BLE_MESH_DEV_UUID", "Completed, UUID: ["UUIDSTR"]", UUID2STR(dev_uuid));
}


static esp_err_t ble_mesh_init(void)
{
    const char *TAG = "BLE_MESH_INIT";
    uint8_t match[2] = {0x32, 0x10};
    esp_err_t ret = ESP_OK;

    uint8_t net_key[16] = NET_KEY;
    prov_key.net_idx = NET_KEY_IDX;   //0x0000
    prov_key.app_idx = APP_KEY_IDX;   //0x0000
    memset(prov_key.app_key, APP_KEY, sizeof(prov_key.app_key));  //0x12
    memcpy(prov_key.net_key, net_key, sizeof(prov_key.net_key));  //0x12

    ESP_ERR_FUNC_RET(esp_ble_mesh_register_prov_callback(ble_mesh_provisioning_cb),
                     "Failed to register provisioning callback");
    ESP_ERR_FUNC_RET(esp_ble_mesh_register_config_client_callback(ble_mesh_config_client_cb),
                     "Failed to register config client callback");
    ESP_ERR_FUNC_RET(esp_ble_mesh_register_custom_model_callback(ble_mesh_my_model_cb),
                     "Failed to register custom model callback");
    ESP_ERR_FUNC_RET(esp_ble_mesh_register_generic_client_callback(ble_mesh_generic_client_cb),
                     "Failed to register generic client callback");

    ESP_LOGI(TAG, "Initializing mesh stack");
    ESP_ERR_FUNC_RET(esp_ble_mesh_init(&provision, &composition),
                     "Failed to initialize mesh stack");

    ESP_LOGI(TAG, "Initializing vendor client");
    ESP_ERR_FUNC_RET(esp_ble_mesh_client_model_init(&vnd_models[0]),
                     "Failed to initialize vendor client");

    ESP_LOGI(TAG, "Setting matching device uuid");
    ESP_ERR_FUNC_RET(esp_ble_mesh_provisioner_set_dev_uuid_match(match, sizeof(match), 0x0, false),
                     "Failed to set matching device uuid");

    ESP_LOGI(TAG, "Enabling mesh provisioner");
    ESP_ERR_FUNC_RET(esp_ble_mesh_provisioner_prov_enable((esp_ble_mesh_prov_bearer_t)
                     (ESP_BLE_MESH_PROV_ADV | ESP_BLE_MESH_PROV_GATT)),
                     "Failed to enable mesh provisioner");

    ESP_LOGI(TAG, "Adding local net key");
    ESP_ERR_FUNC_RET(esp_ble_mesh_provisioner_update_local_net_key(
                     prov_key.net_key, prov_key.net_idx),
                     "Failed to add local NetKey");

    ESP_LOGI(TAG, "Adding local app key");
    ESP_ERR_FUNC_RET(esp_ble_mesh_provisioner_add_local_app_key(
                     prov_key.app_key, prov_key.net_idx, prov_key.app_idx),
                     "Failed to add local AppKey");


    ESP_LOGI(TAG, "Provisioner initialized");

    // ble_mesh_node_init(node_info.server_addr);
    // write_enable = true;
    return ret;
}

static void hs_reset_cb(int reason)
{
    ESP_LOGI("BLE_HOST_RESET", "Resetting state; reason=%d\n", reason);
}

static esp_err_t hs_sync_cb(void)
{
    esp_err_t ret = ESP_OK;

    ESP_ERR_FUNC_RET(ble_hs_util_ensure_addr(0), 
                    "Failed to ensure address");
    ESP_ERR_FUNC_RET(ble_hs_id_infer_auto(0, &own_addr_type), 
                    "Failed to infer address type");
    ESP_ERR_FUNC_RET(ble_hs_id_copy_addr(own_addr_type, addr_val, NULL), 
                    "Failed to copy address");

    ESP_LOGI("BLE_HOST_SYNC", "Synced, Addr: ["MACSTR"]", MAC2STR(addr_val));
    xSemaphoreGive(mesh_sem);

    return ret;
}

void hs_task(void *pv)
{
    ESP_LOGI("BLE_HOST_TASK", "Task Started");
    xTaskCreate(ble_send_task, "BLE Send Task", 2048, NULL, 1, NULL);
    nimble_port_run();
    nimble_port_freertos_deinit(); 
}


void app_main(void)
{
    if (mesh_sem == NULL) {
        mesh_sem = xSemaphoreCreateBinary();
    }

    ESP_ERROR_CHECK(nvs_helper_init());

    touch_myinit();

    ESP_ERROR_CHECK(nimble_port_init());

    ble_hs_cfg.reset_cb = hs_reset_cb;
    ble_hs_cfg.sync_cb  = hs_sync_cb;
    ble_hs_cfg.store_status_cb = ble_store_util_status_rr;    
    ble_store_config_init();

    nimble_port_freertos_init(hs_task);

    if (xSemaphoreTake(mesh_sem, portMAX_DELAY) == pdTRUE) {
        ble_mesh_get_dev_uuid(dev_uuid);
    }

    ESP_ERROR_CHECK(ble_mesh_init());

}

