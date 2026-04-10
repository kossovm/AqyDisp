#include "fetchData.h"
#include <string.h>
#include <stdlib.h>
#include "esp_log.h"
#include "esp_http_client.h"
#include "cJSON.h" // provided by ESP-IDF's built-in cJSON component
#include "esp_crt_bundle.h" // Needed for HTTPS certification

static const char *TAG = "FETCH_API";

#define MAX_HTTP_OUTPUT_BUFFER 2048

// API URL
#define API_URL "https://api.coffeeh.kz/api/v1/displays/pickup/1"

/**
 * @brief Event handler for the HTTP client. 
 * Accumulates the response data into a buffer.
 */
esp_err_t _http_event_handler(esp_http_client_event_t *evt)
{
    static char *output_buffer;  // Buffer to store response of http request
    static int output_len;       // Stores number of bytes read

    switch(evt->event_id) {
        case HTTP_EVENT_ON_DATA:
            // Append new data to the buffer
            if (!esp_http_client_is_chunked_response(evt->client)) {
                // If user_data buffer is configured, copy data there
                if (evt->user_data) {
                    memcpy(evt->user_data + output_len, evt->data, evt->data_len);
                } else {
                     // If you didn't pass a buffer pointer, you'd handle heap allocation here
                     // But for this example, we will use the user_data approach passed from the main function
                }
                output_len += evt->data_len;
            }
            break;
        case HTTP_EVENT_ON_FINISH:
            output_len = 0;
            break;
        case HTTP_EVENT_DISCONNECTED:
            output_len = 0;
            break;
        default:
            break;
    }
    return ESP_OK;
}

esp_err_t fetch_order_data(OrderData *out_data)
{
    // 1. Prepare a buffer to hold the JSON string
    char *response_buffer = (char *)malloc(MAX_HTTP_OUTPUT_BUFFER);
    if (response_buffer == NULL) {
        ESP_LOGE(TAG, "Failed to allocate memory for HTTP response");
        return ESP_ERR_NO_MEM;
    }
    // Clear buffer
    memset(response_buffer, 0, MAX_HTTP_OUTPUT_BUFFER);

    // 2. Configure HTTP Client
    esp_http_client_config_t config = {
        .url = API_URL,
        .event_handler = _http_event_handler,
        .user_data = response_buffer,        // Pass buffer to event handler
        .disable_auto_redirect = true,
        // IMPORTANT for HTTPS: Attach default certificate bundle or skip verification
        // Option A: Use ESP-IDF default bundle (Recommended)
        .crt_bundle_attach = esp_crt_bundle_attach, 
        // Option B: If Option A fails, uncomment the line below to skip verification (Insecure)
        // .skip_cert_common_name_check = true, 
    };

    esp_http_client_handle_t client = esp_http_client_init(&config);
    
    // 3. Perform the GET request
    esp_err_t err = esp_http_client_perform(client);

    bool parse_success = false;

    if (err == ESP_OK) {
        ESP_LOGI(TAG, "HTTP GET Status = %d, content_length = %lld",
                 esp_http_client_get_status_code(client),
                 esp_http_client_get_content_length(client));

        // 4. Parse JSON
        cJSON *root = cJSON_Parse(response_buffer);
        if (root == NULL) {
            ESP_LOGE(TAG, "JSON Parse Error");
        } else {
            // Navigate to "order" object
            cJSON *order_obj = cJSON_GetObjectItem(root, "order");
            if (order_obj) {
                // Extract order_id
                cJSON *id_item = cJSON_GetObjectItem(order_obj, "order_id");
                if (cJSON_IsNumber(id_item)) {
                    out_data->order_id = id_item->valueint;
                }

                // Extract order_number
                cJSON *num_item = cJSON_GetObjectItem(order_obj, "order_number");
                if (cJSON_IsString(num_item) && (num_item->valuestring != NULL)) {
                    strncpy(out_data->order_number, num_item->valuestring, sizeof(out_data->order_number) - 1);
                }

                // Extract customer_name
                cJSON *name_item = cJSON_GetObjectItem(order_obj, "customer_name");
                if (cJSON_IsString(name_item) && (name_item->valuestring != NULL)) {
                    strncpy(out_data->customer_name, name_item->valuestring, sizeof(out_data->customer_name) - 1);
                }
                
                parse_success = true;
            } else {
                ESP_LOGE(TAG, "JSON object 'order' not found");
            }
            cJSON_Delete(root);
        }
    } else {
        ESP_LOGE(TAG, "HTTP GET request failed: %s", esp_err_to_name(err));
    }

    // Cleanup
    esp_http_client_cleanup(client);
    free(response_buffer);

    return parse_success ? ESP_OK : ESP_FAIL;
}