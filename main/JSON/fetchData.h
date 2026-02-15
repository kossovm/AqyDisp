#ifndef FETCH_DATA_H
#define FETCH_DATA_H

#include <esp_err.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Structure to hold the parsed JSON data
typedef struct {
    int order_id;
    char order_number[32];
    char customer_name[128]; // Big enough to hold Cyrillic/UTF-8
} OrderData;

/**
 * @brief Fetches JSON from the API and parses it into the OrderData struct.
 * 
 * @param out_data Pointer to the OrderData struct to fill.
 * @return esp_err_t ESP_OK on success, ESP_FAIL otherwise.
 */
esp_err_t fetch_order_data(OrderData *out_data);

#ifdef __cplusplus
}
#endif

#endif // FETCH_DATA_H