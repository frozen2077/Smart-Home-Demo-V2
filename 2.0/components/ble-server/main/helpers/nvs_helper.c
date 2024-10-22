#include "string.h"
#include "stdio.h"
#include "stdlib.h"

#include "nvs.h"
#include "nvs_flash.h"
#include "esp_ota_ops.h"

#include "nvs_helper.h"
#include "my_utils.h"

static const char *TAG2 = "NVS HELPER";

esp_err_t nvs_helper_init()
{
    static bool init_flag = false;

    if (!init_flag) {
        esp_err_t ret = nvs_flash_init();

        if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
            // NVS partition was truncated and needs to be erased
            // Retry nvs_flash_init
            ret = nvs_flash_erase();
            
            if(ret == ESP_OK) {
                ret = nvs_flash_init();
            }
        }

        if (ret != ESP_OK) {
            ESP_LOGE(TAG2, "nvs_flash_init, err_str: %s", esp_err_to_name(ret));
            return ret;
            // const esp_partition_t *partition = esp_ota_get_next_update_partition(NULL);

            // if (partition) {
            //     ret = esp_ota_set_boot_partition(partition);
            //     ESP_ERROR_RETURN(ret != ESP_OK, ret, "esp_ota_set_boot_partition failed!");
            //     ESP_LOGI(__func__, "The next reboot will fall back to the previous version");
            // }
        }

        init_flag = true;
    }

    return ESP_OK;
}

esp_err_t nvs_helper_erase(const char *namespace, const char *key)
{
    // ESP_PARAM_CHECK(key);

    esp_err_t ret    = ESP_OK;
    nvs_handle_t handle = 0;

    /**< Open non-volatile storage with a given namespace from the default NVS partition */
    ret = nvs_open(namespace, NVS_READWRITE, &handle);
    ESP_ERROR_RETURN(ret != ESP_OK, ret, "Open non-volatile storage");

    /**
     * @brief If key is CONFIG_ESPNOW_NVS_NAMESPACE, erase all info in CONFIG_ESPNOW_NVS_NAMESPACE
     */
    if (!key) {
        ret = nvs_erase_all(handle);
    } else {
        ret = nvs_erase_key(handle, key);
    }

    /**< Write any pending changes to non-volatile storage */
    nvs_commit(handle);

    /**< Close the storage handle and free any allocated resources */
    nvs_close(handle);

    ESP_ERROR_RETURN(ret != ESP_OK && ret != ESP_ERR_NVS_NOT_FOUND,
                     ret, "Erase key-value pair, key: %s", key);

    return ESP_OK;
}

esp_err_t nvs_helper_blob_set(const char *namespace, const char *key, const void *value, size_t length)
{
    ESP_PARAM_CHECK(key);
    ESP_PARAM_CHECK(value);
    ESP_PARAM_CHECK(length > 0);

    esp_err_t ret     = ESP_OK;
    nvs_handle_t handle = 0;

    /**< Open non-volatile storage with a given namespace from the default NVS partition */
    ret = nvs_open(namespace, NVS_READWRITE, &handle);
    ESP_ERROR_RETURN(ret != ESP_OK, ret, "Open non-volatile storage");

    /**< set variable length binary value for given key */
    ret = nvs_set_blob(handle, key, value, length);

    /**< Write any pending changes to non-volatile storage */
    nvs_commit(handle);

    /**< Close the storage handle and free any allocated resources */
    nvs_close(handle);

    ESP_ERROR_RETURN(ret != ESP_OK, ret, "Set value for given key, key: %s", key);

    return ESP_OK;
}

esp_err_t nvs_helper_blob_get(const char *namespace, const char *key, void *value, size_t length)
{
    ESP_PARAM_CHECK(key);
    ESP_PARAM_CHECK(value);
    // ESP_PARAM_CHECK(length > 0);

    esp_err_t ret     = ESP_OK;
    nvs_handle_t handle = 0;

    /**< Open non-volatile storage with a given namespace from the default NVS partition */
    ret = nvs_open(namespace, NVS_READWRITE, &handle);
    ESP_ERROR_RETURN(ret != ESP_OK, ret, "Open non-volatile storage");

    if (length == 0) {
        ret = nvs_get_blob(handle, key, NULL, &length);
    }

    /**< get variable length binary value for given key */
    ret = nvs_get_blob(handle, key, value, &length);

    /**< Close the storage handle and free any allocated resources */
    nvs_close(handle);

    if (ret == ESP_ERR_NVS_NOT_FOUND) {
        ESP_LOGD(TAG2, "<ESP_ERR_NVS_NOT_FOUND> Get value for given key, key: %s", key);
        return ESP_ERR_NVS_NOT_FOUND;
    }

    ESP_ERROR_RETURN(ret != ESP_OK, ret, "Get value for given key, key: %s", key);

    return ESP_OK;
}
