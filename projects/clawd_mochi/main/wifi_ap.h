#pragma once
#include "esp_err.h"
#include <stdbool.h>

esp_err_t wifi_ap_init(void);
bool wifi_sta_is_connected(void);
const char *wifi_sta_get_ip(void);
