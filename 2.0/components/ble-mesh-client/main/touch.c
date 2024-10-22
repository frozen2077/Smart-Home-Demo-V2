#include <stdio.h>
#include <string.h>
#include <inttypes.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_log.h"
#include "driver/touch_pad.h"

static const char* TOUCH_TAG = "Touch";

static TaskHandle_t touch_read_handle = NULL;;

#define TOUCH_BUTTON_NUM    3

static QueueHandle_t que_touch = NULL;
typedef struct touch_msg {
    uint8_t pad_num;
} touch_event_t;

static const touch_pad_t button[TOUCH_BUTTON_NUM] = {
    TOUCH_PAD_NUM0,
    TOUCH_PAD_NUM7,     // 'SELECT' button.
    TOUCH_PAD_NUM9,     // 'MENU' button.
};

static const float button_threshold[TOUCH_BUTTON_NUM] = {
    0.7, // 20%.
    0.7, // 20%.
    0.7, // 20%.
};

extern esp_err_t ble_mesh_send_vendor_message(uint16_t dst_addr, char* msg, uint16_t len);
extern void ble_mesh_send_gen_onoff_set(void);

static void touch_set_thresholds(void)
{
    uint16_t touch_value;
    for (int i = 0; i < TOUCH_BUTTON_NUM; i++) {
        //read benchmark value, After initialization, the benchmark value is the maximum during the first measurement period.
        touch_pad_read_filtered(button[i], &touch_value);
        //set interrupt threshold.
        touch_pad_set_thresh(button[i], touch_value * button_threshold[i]);
        ESP_LOGI(TOUCH_TAG, "touch pad [%d] base %d, thresh %d", 
                 button[i], touch_value, (uint16_t)(touch_value * button_threshold[i]));
    }
}

static void touch_interrupt_cb(void *arg)
{
    int task_awoken = pdFALSE;
    touch_event_t evt;
    uint32_t pad_intr = touch_pad_get_status();
    touch_pad_clear_status();
    ESP_EARLY_LOGI(TOUCH_TAG, "pad_intr: %lu", pad_intr);
    xQueueSendFromISR(que_touch, &pad_intr, &task_awoken);

    if (task_awoken == pdTRUE) {
        portYIELD_FROM_ISR();
    }
}


static void touch_read_fcn(uint8_t pad)
{
    switch(pad) {
    case TOUCH_PAD_NUM0:
        ESP_LOGW(TOUCH_TAG, "TouchSensor [%d] be activated", pad);
        ble_mesh_send_gen_onoff_set();
    break;

    case TOUCH_PAD_NUM7:
        char my_msg[300];
        static int num = 0;

        ESP_LOGW(TOUCH_TAG, "TouchSensor [%d] be activated 0", pad);
        memset(my_msg, 0, sizeof(my_msg));
        snprintf(my_msg, sizeof(my_msg), "hello amazing spider man yes yea joker :%d", num++);
        ble_mesh_send_vendor_message((uint16_t)0x0005, my_msg, strlen(my_msg));
        break;

    case TOUCH_PAD_NUM9:
        ESP_LOGW(TOUCH_TAG, "TouchSensor [%d] be activated 1", pad);

        char my_msg2[300];
        static int num2 = 0;

        ESP_LOGW(TOUCH_TAG, "TouchSensor [%d] be activated 0", pad);
        memset(my_msg2, 0, sizeof(my_msg2));
        snprintf(my_msg2, sizeof(my_msg2), "This is your boss 001, shut the fuck up!:%d", num2++);
        ble_mesh_send_vendor_message((uint16_t)0x0006 ,my_msg2, strlen(my_msg2));               
        break;
    }
}

static void touch_read_task(void *pvParameter)
{
    uint32_t notify_value;

    while (1) {
        if (xQueueReceive(que_touch, &notify_value, portMAX_DELAY) != pdTRUE) {
            continue;
        }

        for (uint8_t i = 0; i < TOUCH_PAD_MAX; i++) {
            if ((notify_value >> i) & 0x01) {
                touch_read_fcn(i);
            }
        }
        vTaskDelay(5000 / portTICK_PERIOD_MS);
        xQueueReset(que_touch);
    }
}


void touch_myinit()
{
    if (que_touch == NULL) {
        que_touch = xQueueCreate(1, sizeof(uint32_t));
    }
    ESP_LOGI(TOUCH_TAG, "Initializing touch pad");
    // Initialize touch pad peripheral, it will start a timer to run a filter
    ESP_ERROR_CHECK(touch_pad_init());

    ESP_ERROR_CHECK(touch_pad_set_fsm_mode(TOUCH_FSM_MODE_TIMER));
    ESP_ERROR_CHECK(touch_pad_set_voltage(TOUCH_HVOLT_2V7, TOUCH_LVOLT_0V5, TOUCH_HVOLT_ATTEN_1V));
    touch_pad_set_meas_time(0x6000, 0x6000);

    for (int i = 0; i < TOUCH_BUTTON_NUM; i++) {
        touch_pad_config(button[i], 0);
    }

    ESP_ERROR_CHECK(touch_pad_filter_start(20));

    touch_set_thresholds();

    ESP_ERROR_CHECK(touch_pad_isr_register(touch_interrupt_cb, NULL));
    ESP_ERROR_CHECK(touch_pad_intr_enable());

    xTaskCreate(&touch_read_task, "touch_pad_read_task", 4096, NULL, 1, &touch_read_handle);
    ESP_LOGI(TOUCH_TAG, "Touch pad system initialized");
}

