#include "wifi_ap.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include <string.h>

static const char *TAG = "WIFI";

#define AP_SSID "ClaWD-Mochi"
#define AP_PASS "clawd1234"
#define AP_CHANNEL 1
#define AP_MAX_CONN 4

// Home WiFi to connect to (so Claude Code hooks can reach ESP32)
#define STA_SSID "iPhone 17"
#define STA_PASS "88888888"

static esp_netif_t *sta_netif = NULL;
static volatile bool sta_connected = false;
static char sta_ip_str[16] = {0};

static void wifi_event_handler(void *arg, esp_event_base_t event_base,
                               int32_t event_id, void *event_data)
{
    if (event_base == WIFI_EVENT) {
        switch (event_id) {
            case WIFI_EVENT_AP_STACONNECTED: {
                wifi_event_ap_staconnected_t *event = (wifi_event_ap_staconnected_t *)event_data;
                ESP_LOGI(TAG, "AP: Station connected, AID=%d", event->aid);
                break;
            }
            case WIFI_EVENT_AP_STADISCONNECTED: {
                wifi_event_ap_stadisconnected_t *event = (wifi_event_ap_stadisconnected_t *)event_data;
                ESP_LOGI(TAG, "AP: Station disconnected, AID=%d", event->aid);
                break;
            }
            case WIFI_EVENT_STA_START:
                ESP_LOGI(TAG, "STA: Connecting to '%s'...", STA_SSID);
                esp_wifi_connect();
                break;
            case WIFI_EVENT_STA_DISCONNECTED:
                ESP_LOGI(TAG, "STA: Disconnected, reconnecting...");
                sta_connected = false;
                sta_ip_str[0] = '\0';
                esp_wifi_connect();
                break;
            default:
                break;
        }
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        snprintf(sta_ip_str, sizeof(sta_ip_str), IPSTR, IP2STR(&event->ip_info.ip));
        sta_connected = true;
        ESP_LOGI(TAG, "STA: Got IP: %s", sta_ip_str);
        ESP_LOGI(TAG, "STA: Claude Code hooks → http://%s", sta_ip_str);
    }
}

esp_err_t wifi_ap_init(void)
{
    ESP_LOGI(TAG, "Initializing WiFi STA...");

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    sta_netif = esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL, NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL, NULL));

    // STA config only
    wifi_config_t sta_config = {
        .sta = {
            .ssid = STA_SSID,
            .password = STA_PASS,
            .threshold.authmode = WIFI_AUTH_WPA2_PSK,
        },
    };

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &sta_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "STA: Connecting to '%s'...", STA_SSID);

    return ESP_OK;
}

bool wifi_sta_is_connected(void)
{
    return sta_connected;
}

const char *wifi_sta_get_ip(void)
{
    return sta_ip_str;
}
