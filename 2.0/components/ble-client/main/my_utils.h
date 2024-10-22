#pragma once

#include <stdint.h>

#include "freertos/FreeRTOS.h"

#include <esp_err.h>
#include "esp_log.h"


#define MIN(a,b)    ((a)<(b)?(a):(b))

#ifndef CONFIG_ESPNOW_MEM_DBG_INFO_MAX
#define CONFIG_ESPNOW_MEM_DBG_INFO_MAX     (256)
#endif  /**< CONFIG_ESPNOW_MEM_DBG_INFO_MAX */
#define MEM_DBG_INFO_MAX CONFIG_ESPNOW_MEM_DBG_INFO_MAX

#ifdef CONFIG_ESPNOW_MEM_ALLOCATION_SPIRAM
#define MALLOC_CAP_INDICATE MALLOC_CAP_SPIRAM
#else
#define MALLOC_CAP_INDICATE MALLOC_CAP_DEFAULT
#endif

/**
 * @brief Macro serves similar purpose as ``assert``, except that it checks `esp_err_t`
 *        value rather than a `bool` condition. If the argument of `ESP_ERROR_ASSERT`
 *        is not equal `ESP_OK`, then an error message is printed on the console,
 *         and `abort()` is called.
 *
 * @note If `IDF monitor` is used, addresses in the backtrace will be converted
 *       to file names and line numbers.
 *
 * @param[in]  err [description]
 * @return         [description]
 */
#define ESP_ERROR_ASSERT(err) do { \
        esp_err_t __err_rc = (err); \
        if (__err_rc != ESP_OK) { \
            ESP_LOGW(TAG, "[%s, %d] <%s> ESP_ERROR_ASSERT failed, at 0x%08x, expression: %s", \
                     __func__, __LINE__, esp_err_to_name(__err_rc), (intptr_t)__builtin_return_address(0) - 3, __ASSERT_FUNC); \
            assert(0 && #err); \
        } \
    } while(0)

#define ESP_ERROR_GOTO(con, lable, format, ...) do { \
        if (con) { \
            if(*format != '\0') \
                ESP_LOGW(TAG, "[%s, %d]" format, __func__, __LINE__, ##__VA_ARGS__); \
            goto lable; \
        } \
    } while(0)

#define ESP_ERROR_CONTINUE(con, format, ...) { \
        if (con) { \
            if(*format != '\0') \
                ESP_LOGW(TAG, "[%s, %d]: " format, __func__, __LINE__, ##__VA_ARGS__); \
            continue; \
        } \
    }

#define ESP_ERROR_BREAK(con, format, ...) { \
        if (con) { \
            if(*format != '\0') \
                ESP_LOGW(TAG, "[%s, %d]: " format, __func__, __LINE__, ##__VA_ARGS__); \
            break; \
        } \
    }

#define ESP_ERROR_RETURN(con, err, format, ...) do { \
        if (con) { \
            if(*format != '\0') \
                ESP_LOGW(TAG, "[%s, %d] <%s> " format, __func__, __LINE__,esp_err_to_name(err), ##__VA_ARGS__); \
            return err; \
        } \
    } while(0)

#define ESP_PARAM_CHECK(con) do { \
        if (!(con)) { \
            ESP_LOGE(TAG, "[%s, %d]: <ESP_ERR_INVALID_ARG> !(%s)", __func__, __LINE__, #con); \
            return ESP_ERR_INVALID_ARG; \
        } \
    } while(0)

#define ESP_LOG_INFO(con, format, ...) do { \
    esp_err_t err = (con); \
    if (!(err)) { \
        ESP_LOGD(TAG, "[%s, %d] <%s> ", __func__, __LINE__, esp_err_to_name(err)); \
    } else { \
        ESP_LOGW(TAG, "[%s, %d] <%s> " format, __func__, __LINE__, esp_err_to_name(err), ##__VA_ARGS__); \
    } \
    } while(0)    

#define ESP_ERR_FUNC_RET(con, format, ...) do { \
    esp_err_t err = (con); \
    if (!(err)) { \
        ESP_LOGD(TAG, "[%s, %d] <%s> ", __func__, __LINE__, esp_err_to_name(err)); \
    } else { \
        ESP_LOGW(TAG, "[%s, %d] <%s> " format, __func__, __LINE__, esp_err_to_name(err), ##__VA_ARGS__); \
        return err; \
    } \
    } while(0)    

