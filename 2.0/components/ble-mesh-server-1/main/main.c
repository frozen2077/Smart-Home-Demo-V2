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
#include "esp_ble_mesh_local_data_operation_api.h"

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

#include "helpers/nvs_helper.c"
#include "helpers/mem_helper.c"
#include "my_utils.h"

#define CID_ESP             0x02E5

#define MSG_SEND_TTL        3
#define MSG_TIMEOUT         0
#define MSG_ROLE            ROLE_PROVISIONER

#define BLE_MESH_MAX_MSG_LEN    255

// #define MSG_SEND_TTL        3
// #define MSG_TIMEOUT         0
// #define MSG_ROLE            ROLE_PROVISIONER

// #define COMP_DATA_PAGE_0    0x00

// #define COMP_DATA_1_OCTET(msg, offset)      (msg[offset])
// #define COMP_DATA_2_OCTET(msg, offset)      (msg[offset + 1] << 8 | msg[offset])

#define ESP_BLE_MESH_VND_MODEL_ID_CLIENT    0x0000
#define ESP_BLE_MESH_VND_MODEL_ID_SERVER    0x0001

#define ESP_BLE_MESH_VND_MODEL_OP_SEND      ESP_BLE_MESH_MODEL_OP_3(0x00, CID_ESP)
#define ESP_BLE_MESH_VND_MODEL_OP_ACK    ESP_BLE_MESH_MODEL_OP_3(0x01, CID_ESP)

#define UUIDSTR(u)  "%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x%02x%02x"
#define UUID2STR(u) u[0], u[1], u[2], u[3], u[4], u[5], u[6], u[7], \
                    u[8], u[9], u[10], u[11], u[12], u[13], u[14], u[15]

static SemaphoreHandle_t mesh_sem;
static uint8_t own_addr_type;
static uint8_t addr_val[6] = {0};

static nvs_handle_t NVS_HANDLE;
static const char * NVS_KEY = "vendor_client";

static uint8_t dev_uuid[16] = {0x32, 0x10};

static struct esp_ble_mesh_key {
    uint16_t net_idx;
    uint16_t app_idx;
    uint8_t  net_key[16];
    uint8_t  app_key[16];
} prov_key;

static esp_ble_mesh_cfg_srv_t config_server = {
    /* 3 transmissions with 20ms interval */
    .net_transmit = ESP_BLE_MESH_TRANSMIT(1, 20),
    .relay = ESP_BLE_MESH_RELAY_ENABLED,
    .relay_retransmit = ESP_BLE_MESH_TRANSMIT(1, 20),
    .gatt_proxy = ESP_BLE_MESH_GATT_PROXY_DISABLED,
    .beacon = ESP_BLE_MESH_BEACON_DISABLED,
    .friend_state = ESP_BLE_MESH_FRIEND_NOT_SUPPORTED,
    .default_ttl = 7,
};

static const esp_ble_mesh_client_op_pair_t vnd_op_pair[] = {
    {ESP_BLE_MESH_VND_MODEL_OP_SEND, ESP_BLE_MESH_VND_MODEL_OP_ACK},
};

static esp_ble_mesh_client_t vendor_client = {
    .op_pair_size = ARRAY_SIZE(vnd_op_pair),
    .op_pair = vnd_op_pair,
};


static esp_ble_mesh_model_op_t vnd_op[] = {
    {ESP_BLE_MESH_VND_MODEL_OP_SEND,   1, (uint32_t)NULL},
    {ESP_BLE_MESH_VND_MODEL_OP_ACK, 1, (uint32_t)NULL},
    ESP_BLE_MESH_MODEL_OP_END,
};

static esp_ble_mesh_model_t vnd_models[] = {
    ESP_BLE_MESH_VENDOR_MODEL(CID_ESP, ESP_BLE_MESH_VND_MODEL_ID_SERVER,
                              vnd_op, NULL, &vendor_client),
};

ESP_BLE_MESH_MODEL_PUB_DEFINE(onoff_pub, 2 + 3, ROLE_NODE);
static esp_ble_mesh_gen_onoff_srv_t onoff_server = {
    .rsp_ctrl = {
        .get_auto_rsp = ESP_BLE_MESH_SERVER_RSP_BY_APP,
        .set_auto_rsp = ESP_BLE_MESH_SERVER_RSP_BY_APP,
    },
};


static esp_ble_mesh_model_t root_models[] = {
    ESP_BLE_MESH_MODEL_CFG_SRV(&config_server),
    ESP_BLE_MESH_MODEL_GEN_ONOFF_SRV(&onoff_pub, &onoff_server),
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
    .uuid = dev_uuid,
};

void ble_store_config_init(void);
extern void touch_myinit();

#define BLE_FUNC_ERR_RET(con, format, ...) do { \
    int ret = (con); \
        if ((ret) != 0) { \
            if(*format != '\0') \
                ESP_LOGW(TAG, "[%s, %d] <%s> " format, __func__, __LINE__, ble_err2str(ret), ##__VA_ARGS__); \
            return; \
        } \
    } while(0) 

// static void ble_mesh_set_msg_common(esp_ble_mesh_client_common_param_t *common,
//                                             esp_ble_mesh_node_t *node,
//                                             esp_ble_mesh_model_t *model, uint32_t opcode)
// {
//     common->opcode = opcode;
//     common->model = model;
//     common->ctx.net_idx = prov_key.net_idx;
//     common->ctx.app_idx = prov_key.app_idx;
//     common->ctx.addr = node->unicast_addr;
//     common->ctx.send_ttl = MSG_SEND_TTL;
//     common->msg_timeout = MSG_TIMEOUT;
// }

static void ble_handle_gen_onoff_msg(esp_ble_mesh_model_t *model,
                                         esp_ble_mesh_msg_ctx_t *ctx,
                                         esp_ble_mesh_server_recv_gen_onoff_set_t *set)
{
    const char *TAG = "BLE_HANDLE_ONOFF";
    esp_ble_mesh_gen_onoff_srv_t *srv = (esp_ble_mesh_gen_onoff_srv_t *)model->user_data;

    switch (ctx->recv_op) {
    case ESP_BLE_MESH_MODEL_OP_GEN_ONOFF_GET:
        ESP_LOGI(TAG, "GEN_ONOFF_GET");
        esp_ble_mesh_server_model_send_msg(model, ctx,
            ESP_BLE_MESH_MODEL_OP_GEN_ONOFF_STATUS, sizeof(srv->state.onoff), &srv->state.onoff);
        break;
    case ESP_BLE_MESH_MODEL_OP_GEN_ONOFF_SET:
    case ESP_BLE_MESH_MODEL_OP_GEN_ONOFF_SET_UNACK:
        if (set->op_en == false) {
            srv->state.onoff = set->onoff;
        } else {
            /* TODO: Delay and state transition */
            srv->state.onoff = set->onoff;
        }

        if (ctx->recv_op == ESP_BLE_MESH_MODEL_OP_GEN_ONOFF_SET) {
            ESP_LOGI(TAG, "GEN_ONOFF_SET, acking");
            esp_ble_mesh_server_model_send_msg(model, ctx,
                ESP_BLE_MESH_MODEL_OP_GEN_ONOFF_STATUS, sizeof(srv->state.onoff), &srv->state.onoff);
        }

        esp_ble_mesh_model_publish(model, ESP_BLE_MESH_MODEL_OP_GEN_ONOFF_STATUS,
            sizeof(srv->state.onoff), &srv->state.onoff, ROLE_NODE);
        ESP_LOGI(TAG, "Receive set command : [%d]", srv->state.onoff);
        break;
    default:
        break;
    }
}

static void ble_mesh_generic_server_cb(esp_ble_mesh_generic_server_cb_event_t event,
                                        esp_ble_mesh_generic_server_cb_param_t *param)
{
    const char *TAG = "BLE_MESH_GENERIC_SRV_CB";
    esp_err_t ret = ESP_OK;
    ESP_LOGI(TAG, "event 0x%02x, opcode 0x%04" PRIx32 ", src 0x%04x, dst 0x%04x",
        event, param->ctx.recv_op, param->ctx.addr, param->ctx.recv_dst);

    switch (event) {
    case ESP_BLE_MESH_GENERIC_SERVER_STATE_CHANGE_EVT:
        ESP_LOGI(TAG, "State Change Event");
        if (param->ctx.recv_op == ESP_BLE_MESH_MODEL_OP_GEN_ONOFF_SET ||
            param->ctx.recv_op == ESP_BLE_MESH_MODEL_OP_GEN_ONOFF_SET_UNACK) {
            ESP_LOGI(TAG, "OnOff Set: %d", param->value.state_change.onoff_set.onoff);
        }
        break;
    case ESP_BLE_MESH_GENERIC_SERVER_RECV_GET_MSG_EVT:
        ESP_LOGI(TAG, "Receive Get Message Event");
        if (param->ctx.recv_op == ESP_BLE_MESH_MODEL_OP_GEN_ONOFF_GET) {
            esp_ble_mesh_gen_onoff_srv_t *srv = (esp_ble_mesh_gen_onoff_srv_t *)param->model->user_data;
            ESP_LOGI(TAG, "OnOff: %d", srv->state.onoff);
            ble_handle_gen_onoff_msg(param->model, &param->ctx, NULL);
        }
        break;
    case ESP_BLE_MESH_GENERIC_SERVER_RECV_SET_MSG_EVT:
        ESP_LOGI(TAG, "Receive Set Message Event");
        if (param->ctx.recv_op == ESP_BLE_MESH_MODEL_OP_GEN_ONOFF_SET ||
            param->ctx.recv_op == ESP_BLE_MESH_MODEL_OP_GEN_ONOFF_SET_UNACK) {
            ESP_LOGI(TAG, "OnOff: %d, TID: %d", param->value.set.onoff.onoff, param->value.set.onoff.tid);
            ble_handle_gen_onoff_msg(param->model, &param->ctx, &param->value.set.onoff);
        }
        break;
    default:
        ESP_LOGE(TAG, "Unknown Generic Server event: 0x%02x", event);
        break;
    }
}


static esp_err_t ble_mesh_prov_complete(uint16_t net_idx, uint16_t addr, 
                                        uint8_t flags, uint32_t iv_index)
{
    const char *TAG = "BLE_MESH_PROV_COMPLETE";

    ESP_LOGI(TAG, "Net_idx: [0x%03x] | Addr: [0x%04x] | Flags: [0x%02x] | iv_idx: [0x%08x]", 
             net_idx, addr, flags, (unsigned int)iv_index);

    return ESP_OK;
}


static void ble_mesh_provision_cb(esp_ble_mesh_prov_cb_event_t event,
                                          esp_ble_mesh_prov_cb_param_t *param)
{
    const char *TAG = "BLE_MESH_NODE_PROV_CB";

    switch (event) {
    case ESP_BLE_MESH_PROV_REGISTER_COMP_EVT:
        ESP_LOGI(TAG, "PROV_REGISTER_COMP | Err: >%d<", 
            param->prov_register_comp.err_code);
        break;
    case ESP_BLE_MESH_NODE_PROV_ENABLE_COMP_EVT:
        ESP_LOGI(TAG, "PROV_ENABLE_COMP | Err: >%d<", 
            param->node_prov_enable_comp.err_code);
                        ESP_LOG_INFO(esp_ble_mesh_node_bind_app_key_to_local_model(
                        esp_ble_mesh_get_primary_element_address(), 
                        BLE_MESH_CID_NVAL, ESP_BLE_MESH_MODEL_ID_GEN_ONOFF_SRV, 
                        prov_key.app_idx),
                        "Failed to bind app key to local ON/OFF model");
        break;
    case ESP_BLE_MESH_NODE_PROV_LINK_OPEN_EVT:
        ESP_LOGI(TAG, "PROV_LINK_OPEN | Bearer: >%s<",
            param->node_prov_link_open.bearer == 1 ? "PB-ADV" : "PB-GATT");
        break;
    case ESP_BLE_MESH_NODE_PROV_LINK_CLOSE_EVT:
        ESP_LOGI(TAG, "PROV_LINK_CLOSE | Bearer: >%s<",
            param->node_prov_link_close.bearer == 1 ? "PB-ADV" : "PB-GATT");
        break;
    case ESP_BLE_MESH_NODE_PROV_COMPLETE_EVT:
        ble_mesh_prov_complete(
            param->node_prov_complete.net_idx, 
            param->node_prov_complete.addr,
            param->node_prov_complete.flags, 
            param->node_prov_complete.iv_index);
        break;
    case ESP_BLE_MESH_NODE_PROV_RESET_EVT:
        ESP_LOGI(TAG, "PROV_RESET_EVT");
        break;
    case ESP_BLE_MESH_NODE_SET_UNPROV_DEV_NAME_COMP_EVT:
        ESP_LOGI(TAG, "SET_UNPROV_DEV_NAME_COMP | Err: >%d<", 
            param->node_set_unprov_dev_name_comp.err_code);
        break;
    default:
        break;
    }

    return;
}


static void ble_mesh_config_server_cb(esp_ble_mesh_cfg_server_cb_event_t event,
                                              esp_ble_mesh_cfg_server_cb_param_t *param)
{
    const char *TAG = "BLE_MESH_CFG_SRV_CB";
    esp_err_t ret = ESP_OK;

    switch (event) {
    case ESP_BLE_MESH_CFG_SERVER_STATE_CHANGE_EVT:
        switch (param->ctx.recv_op) {
        case ESP_BLE_MESH_MODEL_OP_APP_KEY_ADD:
            ESP_LOGI(TAG, "APP_KEY_ADD ["UUIDSTR(param->value.state_change.appkey_add.app_key)
                          "] | Net_idx: [0x%04x] | App_idx: [0x%04x]",
                UUID2STR(param->value.state_change.appkey_add.app_key),
                param->value.state_change.appkey_add.net_idx,
                param->value.state_change.appkey_add.app_idx);
            break;
        case ESP_BLE_MESH_MODEL_OP_MODEL_APP_BIND:
            ESP_LOGI(TAG, "MODEL_APP_BIND | elem_addr: [0x%04x] | app_idx: [0x%04x] "
                          "| cid: [0x%04x] | mod_id: [0x%04x]",
                param->value.state_change.mod_app_bind.element_addr,
                param->value.state_change.mod_app_bind.app_idx,
                param->value.state_change.mod_app_bind.company_id,
                param->value.state_change.mod_app_bind.model_id);
                // ESP_ERR_FUNC_RET(esp_ble_mesh_node_bind_app_key_to_local_model(
                //         esp_ble_mesh_get_primary_element_address(), 
                //         BLE_MESH_CID_NVAL, ESP_BLE_MESH_MODEL_ID_GEN_ONOFF_SRV, 
                //         param->value.state_change.mod_app_bind.app_idx),
                //         "Failed to bind app key to local ON/OFF model");
            break;
        case ESP_BLE_MESH_MODEL_OP_MODEL_SUB_ADD:
            ESP_LOGI(TAG, "MODEL_SUB_ADD");
            ESP_LOGI(TAG, "elem_addr 0x%04x, sub_addr 0x%04x, cid 0x%04x, mod_id 0x%04x",
                     param->value.state_change.mod_sub_add.element_addr,
                     param->value.state_change.mod_sub_add.sub_addr,
                     param->value.state_change.mod_sub_add.company_id,
                     param->value.state_change.mod_sub_add.model_id);
            break;

        case ESP_BLE_MESH_MODEL_OP_MODEL_PUB_SET:
            ESP_LOGI(TAG, "MODEL_PUB_SET");
            ESP_LOGI(TAG, "publish address is 0x%04x", param->value.state_change.mod_pub_set.pub_addr);

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

    ESP_ERROR_GOTO(esp_ble_mesh_server_model_send_msg(&vnd_models[0], &ctx, opcode,
                     len, (uint8_t *)my_msg),
                     EXIT, "Failed to send vendor message");

    ESP_LOGI(TAG, "Send message: [%s] len [%d] >>> Srv: [0x%04x]", my_msg, len, ctx.addr);

EXIT:
    free(my_msg);
    return ret;
}

static void ble_mesh_my_model_cb(esp_ble_mesh_model_cb_event_t event,
                                esp_ble_mesh_model_cb_param_t *param)
{

    const char *TAG = "BLE_MESH_MY_MODEL_CB";
    esp_err_t ret = ESP_OK;

    switch (event) {
    case ESP_BLE_MESH_MODEL_OPERATION_EVT:
        if (param->model_operation.opcode == ESP_BLE_MESH_VND_MODEL_OP_SEND) {
            ESP_LOGI(TAG, "Src: [0x%04x] | Dst: [0x%04x] | Recv <OP_SEND> msg : [%s]",
                        param->model_operation.ctx->addr,
                        param->model_operation.ctx->recv_dst,
                        (char *)param->model_operation.msg);
            //Send <OP_ACK> msg
            ESP_LOG_INFO(esp_ble_mesh_server_model_send_msg(&vnd_models[0],
                        param->model_operation.ctx, ESP_BLE_MESH_VND_MODEL_OP_ACK,
                        sizeof("1"), (uint8_t *)"1"),
                        "Failed to send <OP_ACK> message");
        }
        else if (param->model_operation.opcode == ESP_BLE_MESH_VND_MODEL_OP_ACK) {
            ESP_LOGI(TAG, "Src: [0x%04x] | Dst: [0x%04x] | Recv Ack msg : [%s]",
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
}


static esp_err_t ble_mesh_init(void)
{
    const char *TAG = "BLE_MESH_INIT";
    esp_err_t ret = ESP_OK;

    ESP_ERR_FUNC_RET(esp_ble_mesh_register_prov_callback(ble_mesh_provision_cb),
                     "Failed to register provision callback");
    ESP_ERR_FUNC_RET(esp_ble_mesh_register_config_client_callback(ble_mesh_config_server_cb),
                     "Failed to register config client callback");
    ESP_ERR_FUNC_RET(esp_ble_mesh_register_custom_model_callback(ble_mesh_my_model_cb),
                     "Failed to register custom model callback");
    ESP_ERR_FUNC_RET(esp_ble_mesh_register_generic_server_callback(ble_mesh_generic_server_cb),
                     "Failed to register generic server callback");

    ESP_LOGI(TAG, "Initializing mesh stack");
    ESP_ERR_FUNC_RET(esp_ble_mesh_init(&provision, &composition),
                     "Failed to initialize mesh stack");

    ESP_LOGI(TAG, "Provisioning enabling");
    ESP_ERR_FUNC_RET(esp_ble_mesh_node_prov_enable((esp_ble_mesh_prov_bearer_t)
                     (ESP_BLE_MESH_PROV_ADV | ESP_BLE_MESH_PROV_GATT)),
                     "Failed to enable mesh provisioner");

    ESP_LOGI(TAG, "Node initialized");

    return ret;
}

static void hs_reset_cb(int reason)
{
    ESP_LOGI("BLE_HOST_RESET", "Resetting state; reason=%d\n", reason);
}

static void hs_sync_cb(void)
{
    ble_hs_util_ensure_addr(0);
    ble_hs_id_infer_auto(0, &own_addr_type);
    ble_hs_id_copy_addr(own_addr_type, addr_val, NULL);

    xSemaphoreGive(mesh_sem);
}

void hs_task(void *pv)
{
    ESP_LOGI("BLE_HOST_TASK", "Task Started");

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

