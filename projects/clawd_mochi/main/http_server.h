#pragma once
#include "esp_err.h"
#include "esp_http_server.h"

// Callback types for server events
typedef void (*cmd_callback_t)(char key);
typedef void (*char_callback_t)(char c);
typedef void (*speed_callback_t)(int speed);
typedef void (*backlight_callback_t)(bool on);
typedef void (*redraw_callback_t)(const char *bg_hex);
typedef void (*canvas_callback_t)(bool on);
typedef void (*draw_clear_callback_t)(const char *bg_hex);
typedef void (*draw_stroke_callback_t)(const char *pen_hex, const char *pts);

typedef struct {
    cmd_callback_t        on_cmd;
    char_callback_t       on_char;
    speed_callback_t      on_speed;
    backlight_callback_t  on_backlight;
    redraw_callback_t     on_redraw;
    canvas_callback_t     on_canvas;
    draw_clear_callback_t on_draw_clear;
    draw_stroke_callback_t on_draw_stroke;
    // State getters for /state endpoint
    volatile uint8_t *current_view;
    volatile bool    *busy;
    volatile bool    *term_mode;
    volatile bool    *backlight_on;
    volatile uint8_t *anim_speed;
} http_server_config_t;

esp_err_t http_server_start(http_server_config_t *cfg);
void http_server_stop(void);
