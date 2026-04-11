#include "captive_portal.h"
#include "config_app.h"
#include "esp_wifi.h"
#include "esp_netif.h"
#include "esp_http_server.h"
#include "esp_log.h"
#include <string.h>
#include <sys/param.h>

static const char *TAG = "PORTAL";
static httpd_handle_t server = NULL;

static const char* html_template = 
"<!DOCTYPE html><html><head><meta charset=\"utf-8\"><meta name=\"viewport\" content=\"width=device-width,initial-scale=1\">"
"<title>Настройка Дисплея</title><style>"
"body{font-family:sans-serif;background:#f0f2f5;padding:20px;}"
".card{background:white;padding:20px;border-radius:10px;max-width:400px;margin:auto;box-shadow:0 4px 6px rgba(0,0,0,0.1);}"
"input,select{width:100%%;padding:10px;margin:10px 0;box-sizing:border-box;border:1px solid #ccc;border-radius:5px;}"
"button{width:100%%;padding:12px;background:#007bff;color:white;border:none;border-radius:5px;font-size:16px;cursor:pointer;}"
"</style></head><body><div class=\"card\"><h2>Настройка Coffee Display</h2>"
"<form action=\"/save\" method=\"POST\">"
"<label>Wi-Fi Сеть (SSID):</label><input type=\"text\" name=\"ssid\" required value=\"%s\">"
"<label>Пароль:</label><input type=\"password\" name=\"password\" value=\"%s\">"
"<label>ID Дисплея:</label><input type=\"number\" name=\"display_id\" required value=\"%d\">"
"<label>Цветовая Тема:</label><select name=\"theme_id\">"
"<option value=\"1\" %s>1: Разноцветная (По умолчанию)</option>"
"<option value=\"2\" %s>2: Белый текст на черном</option>"
"<option value=\"3\" %s>3: Черный текст на белом</option>"
"</select><button type=\"submit\">Сохранить и перезагрузить</button>"
"</form></div></body></html>";

static esp_err_t get_handler(httpd_req_t *req) {
    char response[2048];
    snprintf(response, sizeof(response), html_template, 
        g_app_config.ssid, g_app_config.password, g_app_config.display_id,
        g_app_config.theme_id == 1 ? "selected" : "",
        g_app_config.theme_id == 2 ? "selected" : "",
        g_app_config.theme_id == 3 ? "selected" : "");
    httpd_resp_send(req, response, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

// Simple url decoding
static void parse_param(const char *body, const char *key, char *out, size_t out_len) {
    out[0] = '\0';
    char search_key[32];
    snprintf(search_key, sizeof(search_key), "%s=", key);
    char *start = strstr(body, search_key);
    if (!start) return;
    start += strlen(search_key);
    char *end = strchr(start, '&');
    size_t len = end ? (size_t)(end - start) : strlen(start);
    if (len >= out_len) len = out_len - 1;
    strncpy(out, start, len);
    out[len] = '\0';

    // Replace '+' with space and rudimentary decode
    for(int i=0; i<len; i++) {
        if(out[i] == '+') out[i] = ' ';
    }
}

static esp_err_t post_save_handler(httpd_req_t *req) {
    char buf[512] = {0};
    int ret = httpd_req_recv(req, buf, MIN(req->content_len, sizeof(buf) - 1));
    if (ret <= 0) return ESP_FAIL;

    char new_ssid[MAX_SSID_LEN];
    char new_pass[MAX_PASS_LEN];
    char new_id[16];
    char new_theme[16];

    parse_param(buf, "ssid", new_ssid, sizeof(new_ssid));
    parse_param(buf, "password", new_pass, sizeof(new_pass));
    parse_param(buf, "display_id", new_id, sizeof(new_id));
    parse_param(buf, "theme_id", new_theme, sizeof(new_theme));

    strcpy(g_app_config.ssid, new_ssid);
    strcpy(g_app_config.password, new_pass);
    if (strlen(new_id) > 0) g_app_config.display_id = atoi(new_id);
    if (strlen(new_theme) > 0) g_app_config.theme_id = atoi(new_theme);

    config_app_save();

    const char* resp = "<html><head><meta charset=\"utf-8\"></head><body style=\"font-family:sans-serif;text-align:center;padding:50px;\"><h2>Сохранено! Устройство перезагружается...</h2></body></html>";
    httpd_resp_send(req, resp, HTTPD_RESP_USE_STRLEN);

    // Give time to send response before restart
    vTaskDelay(pdMS_TO_TICKS(1000));
    esp_restart();

    return ESP_OK;
}

static const httpd_uri_t uri_get = { .uri = "/", .method = HTTP_GET, .handler = get_handler, .user_ctx = NULL };
static const httpd_uri_t uri_post = { .uri = "/save", .method = HTTP_POST, .handler = post_save_handler, .user_ctx = NULL };
// Fallback for captive portal checks (redirect everything to /)
static esp_err_t any_get_handler(httpd_req_t *req) {
    httpd_resp_set_status(req, "302 Found");
    httpd_resp_set_hdr(req, "Location", "/");
    httpd_resp_send(req, NULL, 0);
    return ESP_OK;
}
static const httpd_uri_t uri_any_get = { .uri = "/*", .method = HTTP_GET, .handler = any_get_handler, .user_ctx = NULL };


void start_captive_portal(void) {
    // 1. Initialize softAP (Note: esp_netif_init and event_loop are now in main.c)
    esp_netif_create_default_wifi_ap();
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    wifi_config_t wifi_config = {
        .ap = {
            .ssid = "Coffee_Setup",
            .ssid_len = strlen("Coffee_Setup"),
            .channel = 1,
            .password = "",
            .max_connection = 4,
            .authmode = WIFI_AUTH_OPEN
        },
    };

    esp_wifi_set_mode(WIFI_MODE_AP);
    esp_wifi_set_config(WIFI_IF_AP, &wifi_config);
    esp_wifi_start();

    ESP_LOGI(TAG, "Captive Portal AP started: Coffee_Setup");

    // 2. Start HTTP server
    httpd_config_t http_config = HTTPD_DEFAULT_CONFIG();
    http_config.uri_match_fn = httpd_uri_match_wildcard;

    if (httpd_start(&server, &http_config) == ESP_OK) {
        httpd_register_uri_handler(server, &uri_get);
        httpd_register_uri_handler(server, &uri_post);
        httpd_register_uri_handler(server, &uri_any_get); 
    }
}

void stop_captive_portal(void) {
    if (server) {
        httpd_stop(server);
        server = NULL;
    }
}
