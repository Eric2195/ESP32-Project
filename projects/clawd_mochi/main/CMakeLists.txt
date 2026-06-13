idf_component_register(SRCS "main.c" "display.c" "wifi_ap.c" "http_server.c"
                       INCLUDE_DIRS "."
                       REQUIRES esp_lcd esp_driver_usb_serial_jtag nvs_flash esp_wifi esp_event esp_netif esp_http_server)
