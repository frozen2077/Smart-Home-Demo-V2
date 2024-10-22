#pragma once

#include <esp_err.h>

#ifdef __cplusplus
extern "C"
{
#endif

/** 
 * @brief Intialise ESP Storage
 *
 * @return
 *     - ESP_FAIL
 *     - ESP_OK
 */
esp_err_t nvs_helper_init(void);

/**
 * @brief Save the information with given key
 *
 * @param[in]  key    key name. Maximal length is 15 characters. Shouldn't be empty.
 * @param[in]  value  the value to set
 * @param[in]  length length of binary value to set, in bytes; Maximum length is
 *             1984 bytes or (508000 bytes or (97.6% of the partition size - 4000) bytes
 *             whichever is lower, in case multi-page blob support is enabled).
 *
 * @return
 *     - ESP_FAIL
 *     - ESP_OK
 */
esp_err_t nvs_helper_blob_set(const char *namespace, const char *key, const void *value, size_t length);

/**
 * @brief Load the information with given key
 *
 * @param[in]  key  the corresponding key of the information that want to load
 * @param[out]  value  the corresponding value of key
 * @param[in]  length  the length of the value, can be 0 or no less than the value length
 *
 * @return
 *     - ESP_FAIL
 *     - ESP_OK
 */
esp_err_t nvs_helper_blob_get(const char *namespace, const char *key, void *value, size_t length);

/**
 * @brief  Erase the information with given key
 *
 * @param[in]  key  the corresponding key of the information that want to erase
 *
 * @return
 *     - ESP_FAIL
 *     - ESP_OK
 */
esp_err_t nvs_helper_blob_erase(const char *namespace, const char *key);

#ifdef __cplusplus
}
#endif
