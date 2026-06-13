#pragma once
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_io.h"
#include <stdint.h>
#include <stdbool.h>

#define DISP_W 240
#define DISP_H 240

// GPIO pins for ESP32-C6 (remapped from C3)
#define PIN_SCK   4
#define PIN_MOSI  5
#define PIN_CS    6
#define PIN_DC    1
#define PIN_RST   2
#define PIN_BL    3

esp_err_t display_init(void);
void display_set_backlight(bool on);
void display_flush(int x1, int y1, int x2, int y2, const uint16_t *color_data);
void display_fill_rect(int x, int y, int w, int h, uint16_t color);
void display_fill_screen(uint16_t color);
void display_draw_pixel(int x, int y, uint16_t color);
void display_draw_line(int x0, int y0, int x1, int y1, uint16_t color);
void display_draw_rect(int x, int y, int w, int h, uint16_t color);
void display_draw_circle(int cx, int cy, int r, uint16_t color);
void display_fill_circle(int cx, int cy, int r, uint16_t color);
void display_draw_triangle(int x0, int y0, int x1, int y1, int x2, int y2, uint16_t color);
void display_fill_triangle(int x0, int y0, int x1, int y1, int x2, int y2, uint16_t color);
void display_draw_fast_hline(int x, int y, int w, uint16_t color);
void display_draw_fast_vline(int x, int y, int h, uint16_t color);
void display_draw_char(int x, int y, char c, uint16_t color, uint16_t bg, uint8_t size);
void display_draw_string(int x, int y, const char *str, uint16_t color, uint16_t bg, uint8_t size);
uint16_t display_color565(uint8_t r, uint8_t g, uint8_t b);
