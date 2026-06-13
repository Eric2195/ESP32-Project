#include "display.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_panel_io.h"
#include "esp_log.h"
#include "esp_heap_caps.h"
#include <string.h>
#include <stdlib.h>

static const char *TAG = "DISPLAY";

static esp_lcd_panel_handle_t panel_handle = NULL;
static esp_lcd_panel_io_handle_t io_handle = NULL;
static spi_device_handle_t spi_dev = NULL;

// DMA-capable line buffer for fast flush
#define BUF_LINES 40
static uint16_t *line_buf = NULL;

esp_err_t display_init(void)
{
    ESP_LOGI(TAG, "Initializing SPI bus...");
    spi_bus_config_t bus_cfg = {
        .sclk_io_num = PIN_SCK,
        .mosi_io_num = PIN_MOSI,
        .miso_io_num = -1,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = DISP_W * BUF_LINES * sizeof(uint16_t),
    };
    ESP_ERROR_CHECK(spi_bus_initialize(SPI2_HOST, &bus_cfg, SPI_DMA_CH_AUTO));

    ESP_LOGI(TAG, "Creating panel IO...");
    esp_lcd_panel_io_spi_config_t io_cfg = {
        .cs_gpio_num = PIN_CS,
        .dc_gpio_num = PIN_DC,
        .spi_mode = 0,
        // 16 MHz：继续降低时钟，排除 SPI 信号失真导致的偏色/花屏
        .pclk_hz = 16 * 1000 * 1000,
        .trans_queue_depth = 10,
        .lcd_cmd_bits = 8,
        .lcd_param_bits = 8,
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)SPI2_HOST, &io_cfg, &io_handle));

    ESP_LOGI(TAG, "Creating ST7789 panel...");
    esp_lcd_panel_dev_config_t panel_cfg = {
        .reset_gpio_num = PIN_RST,
        // Adafruit 默认用 RGB（MADCTL RGB 位 = 0），这里也先用 RGB
        .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_RGB,
        // ESP-IDF 默认 big endian，但这块屏可能解析为 little endian 才对
        .data_endian = LCD_RGB_DATA_ENDIAN_LITTLE,
        .bits_per_pixel = 16,
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_st7789(io_handle, &panel_cfg, &panel_handle));

    ESP_ERROR_CHECK(esp_lcd_panel_reset(panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_init(panel_handle));

    // 颜色问题调试记录：
    // - invert(false)+RGB：墨绿色，眼睛变白
    // - invert(true)+RGB：偏白
    // - invert(true)+BGR：更偏白
    // - invert(false)+BGR：墨绿色
    // 怀疑 ESP-IDF 默认 RAMCTRL big endian 与这块屏不匹配，
    // 现改为 data_endian=LITTLE + invert=true 再试。
    esp_lcd_panel_invert_color(panel_handle, true);

    // 不要强制 set_gap(0,0)，让 st7789 驱动使用 240x240 默认的行列偏移。
    // 如果仍有显示偏移或黑边，可尝试下面常见偏移：
    // esp_lcd_panel_set_gap(panel_handle, 0, 80);   // 驱动 IC 实际 240x320 时常见
    // esp_lcd_panel_set_gap(panel_handle, 80, 0);

    // Rotation: swap_xy + mirror 对应横屏。
    // 原代码 mirror(false, true) 对应 Adafruit setRotation(1)。
    // 你反馈图像倒过来，这里改为 mirror(true, false)（相当于横屏转 180°）。
    esp_lcd_panel_swap_xy(panel_handle, true);
    esp_lcd_panel_mirror(panel_handle, true, false);
    esp_lcd_panel_disp_on_off(panel_handle, true);

    // Allocate DMA line buffer
    line_buf = heap_caps_malloc(DISP_W * BUF_LINES * sizeof(uint16_t), MALLOC_CAP_DMA);
    if (!line_buf) {
        ESP_LOGE(TAG, "Failed to allocate line buffer");
        return ESP_ERR_NO_MEM;
    }

    // Setup backlight
    gpio_set_direction(PIN_BL, GPIO_MODE_OUTPUT);
    display_set_backlight(true);

    ESP_LOGI(TAG, "Display initialized: %dx%d", DISP_W, DISP_H);
    return ESP_OK;
}

void display_set_backlight(bool on)
{
    gpio_set_level(PIN_BL, on ? 1 : 0);
}

void display_flush(int x1, int y1, int x2, int y2, const uint16_t *color_data)
{
    esp_lcd_panel_draw_bitmap(panel_handle, x1, y1, x2 + 1, y2 + 1, (void *)color_data);
}

void display_fill_screen(uint16_t color)
{
    display_fill_rect(0, 0, DISP_W, DISP_H, color);
}

void display_fill_rect(int x, int y, int w, int h, uint16_t color)
{
    if (x < 0 || y < 0 || w <= 0 || h <= 0) return;
    if (x + w > DISP_W) w = DISP_W - x;
    if (y + h > DISP_H) h = DISP_H - y;

    // Fill line buffer with color
    int chunk_lines = BUF_LINES;
    for (int i = 0; i < DISP_W * chunk_lines && i < w * chunk_lines; i++) {
        line_buf[i] = color;
    }

    for (int row = 0; row < h; row += chunk_lines) {
        int lines = (h - row < chunk_lines) ? (h - row) : chunk_lines;
        esp_lcd_panel_draw_bitmap(panel_handle, x, y + row, x + w, y + row + lines, line_buf);
    }
}

void display_draw_pixel(int x, int y, uint16_t color)
{
    if (x < 0 || x >= DISP_W || y < 0 || y >= DISP_H) return;
    esp_lcd_panel_draw_bitmap(panel_handle, x, y, x + 1, y + 1, &color);
}

void display_draw_fast_hline(int x, int y, int w, uint16_t color)
{
    display_fill_rect(x, y, w, 1, color);
}

void display_draw_fast_vline(int x, int y, int h, uint16_t color)
{
    display_fill_rect(x, y, 1, h, color);
}

void display_draw_line(int x0, int y0, int x1, int y1, uint16_t color)
{
    int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
    int dy = -abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
    int err = dx + dy, e2;

    for (;;) {
        display_draw_pixel(x0, y0, color);
        if (x0 == x1 && y0 == y1) break;
        e2 = 2 * err;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
}

void display_draw_rect(int x, int y, int w, int h, uint16_t color)
{
    display_draw_fast_hline(x, y, w, color);
    display_draw_fast_hline(x, y + h - 1, w, color);
    display_draw_fast_vline(x, y, h, color);
    display_draw_fast_vline(x + w - 1, y, h, color);
}

void display_draw_circle(int cx, int cy, int r, uint16_t color)
{
    int x = r, y = 0, err = 1 - r;
    while (x >= y) {
        display_draw_pixel(cx + x, cy + y, color);
        display_draw_pixel(cx + y, cy + x, color);
        display_draw_pixel(cx - y, cy + x, color);
        display_draw_pixel(cx - x, cy + y, color);
        display_draw_pixel(cx - x, cy - y, color);
        display_draw_pixel(cx - y, cy - x, color);
        display_draw_pixel(cx + y, cy - x, color);
        display_draw_pixel(cx + x, cy - y, color);
        y++;
        if (err <= 0) { err += 2 * y + 1; }
        else { x--; err += 2 * (y - x) + 1; }
    }
}

void display_fill_circle(int cx, int cy, int r, uint16_t color)
{
    display_draw_fast_vline(cx, cy - r, 2 * r + 1, color);
    int x = r, y = 0, err = 1 - r;
    while (x >= y) {
        display_draw_fast_vline(cx + x, cy - y, 2 * y + 1, color);
        display_draw_fast_vline(cx + y, cy - x, 2 * x + 1, color);
        display_draw_fast_vline(cx - y, cy - x, 2 * x + 1, color);
        display_draw_fast_vline(cx - x, cy - y, 2 * y + 1, color);
        y++;
        if (err <= 0) { err += 2 * y + 1; }
        else { x--; err += 2 * (y - x) + 1; }
    }
}

void display_draw_triangle(int x0, int y0, int x1, int y1, int x2, int y2, uint16_t color)
{
    display_draw_line(x0, y0, x1, y1, color);
    display_draw_line(x1, y1, x2, y2, color);
    display_draw_line(x2, y2, x0, y0, color);
}

void display_fill_triangle(int x0, int y0, int x1, int y1, int x2, int y2, uint16_t color)
{
    int16_t a, b, y, last;
    // Sort by y
    if (y0 > y1) { int t = y0; y0 = y1; y1 = t; t = x0; x0 = x1; x1 = t; }
    if (y1 > y2) { int t = y1; y1 = y2; y2 = t; t = x1; x1 = x2; x2 = t; }
    if (y0 > y1) { int t = y0; y0 = y1; y1 = t; t = x0; x0 = x1; x1 = t; }

    if (y0 == y2) {  // degenerate
        a = b = x0;
        if (x1 < a) a = x1; else if (x1 > b) b = x1;
        if (x2 < a) a = x2; else if (x2 > b) b = x2;
        display_draw_fast_hline(a, y0, b - a + 1, color);
        return;
    }

    int16_t dx01 = x1 - x0, dy01 = y1 - y0;
    int16_t dx02 = x2 - x0, dy02 = y2 - y0;
    int16_t dx12 = x2 - x1, dy12 = y2 - y1;
    int32_t sa = 0, sb = 0;

    if (y1 == y2) last = y1;
    else last = y1 - 1;

    for (y = y0; y <= last; y++) {
        a = x0 + sa / dy01;
        b = x0 + sb / dy02;
        sa += dx01;
        sb += dx02;
        if (a > b) { int t = a; a = b; b = t; }
        display_draw_fast_hline(a, y, b - a + 1, color);
    }

    sa = (int32_t)dx12 * (y - y1);
    sb = (int32_t)dx02 * (y - y0);
    for (; y <= y2; y++) {
        a = x1 + sa / dy12;
        b = x0 + sb / dy02;
        sa += dx12;
        sb += dx02;
        if (a > b) { int t = a; a = b; b = t; }
        display_draw_fast_hline(a, y, b - a + 1, color);
    }
}

// Simple 5x8 font (ASCII 32-127)
static const uint8_t font5x8[][5] = {
    {0x00,0x00,0x00,0x00,0x00}, // ' ' 32
    {0x00,0x00,0x5F,0x00,0x00}, // '!' 33
    {0x00,0x07,0x00,0x07,0x00}, // '"' 34
    {0x14,0x7F,0x14,0x7F,0x14}, // '#' 35
    {0x24,0x2A,0x7F,0x2A,0x12}, // '$' 36
    {0x23,0x13,0x08,0x64,0x62}, // '%' 37
    {0x36,0x49,0x55,0x22,0x50}, // '&' 38
    {0x00,0x05,0x03,0x00,0x00}, // ''' 39
    {0x00,0x1C,0x22,0x41,0x00}, // '(' 40
    {0x00,0x41,0x22,0x1C,0x00}, // ')' 41
    {0x14,0x08,0x3E,0x08,0x14}, // '*' 42
    {0x08,0x08,0x3E,0x08,0x08}, // '+' 43
    {0x00,0x50,0x30,0x00,0x00}, // ',' 44
    {0x08,0x08,0x08,0x08,0x08}, // '-' 45
    {0x00,0x60,0x60,0x00,0x00}, // '.' 46
    {0x20,0x10,0x08,0x04,0x02}, // '/' 47
    {0x3E,0x51,0x49,0x45,0x3E}, // '0' 48
    {0x00,0x42,0x7F,0x40,0x00}, // '1' 49
    {0x42,0x61,0x51,0x49,0x46}, // '2' 50
    {0x21,0x41,0x45,0x4B,0x31}, // '3' 51
    {0x18,0x14,0x12,0x7F,0x10}, // '4' 52
    {0x27,0x45,0x45,0x45,0x39}, // '5' 53
    {0x3C,0x4A,0x49,0x49,0x30}, // '6' 54
    {0x01,0x71,0x09,0x05,0x03}, // '7' 55
    {0x36,0x49,0x49,0x49,0x36}, // '8' 56
    {0x06,0x49,0x49,0x29,0x1E}, // '9' 57
    {0x00,0x36,0x36,0x00,0x00}, // ':' 58
    {0x00,0x56,0x36,0x00,0x00}, // ';' 59
    {0x08,0x14,0x22,0x41,0x00}, // '<' 60
    {0x14,0x14,0x14,0x14,0x14}, // '=' 61
    {0x00,0x41,0x22,0x14,0x08}, // '>' 62
    {0x02,0x01,0x51,0x09,0x06}, // '?' 63
    {0x32,0x49,0x79,0x41,0x3E}, // '@' 64
    {0x7E,0x11,0x11,0x11,0x7E}, // 'A' 65
    {0x7F,0x49,0x49,0x49,0x36}, // 'B' 66
    {0x3E,0x41,0x41,0x41,0x22}, // 'C' 67
    {0x7F,0x41,0x41,0x22,0x1C}, // 'D' 68
    {0x7F,0x49,0x49,0x49,0x41}, // 'E' 69
    {0x7F,0x09,0x09,0x09,0x01}, // 'F' 70
    {0x3E,0x41,0x49,0x49,0x7A}, // 'G' 71
    {0x7F,0x08,0x08,0x08,0x7F}, // 'H' 72
    {0x00,0x41,0x7F,0x41,0x00}, // 'I' 73
    {0x20,0x40,0x41,0x3F,0x01}, // 'J' 74
    {0x7F,0x08,0x14,0x22,0x41}, // 'K' 75
    {0x7F,0x40,0x40,0x40,0x40}, // 'L' 76
    {0x7F,0x02,0x0C,0x02,0x7F}, // 'M' 77
    {0x7F,0x04,0x08,0x10,0x7F}, // 'N' 78
    {0x3E,0x41,0x41,0x41,0x3E}, // 'O' 79
    {0x7F,0x09,0x09,0x09,0x06}, // 'P' 80
    {0x3E,0x41,0x51,0x21,0x5E}, // 'Q' 81
    {0x7F,0x09,0x19,0x29,0x46}, // 'R' 82
    {0x46,0x49,0x49,0x49,0x31}, // 'S' 83
    {0x01,0x01,0x7F,0x01,0x01}, // 'T' 84
    {0x3F,0x40,0x40,0x40,0x3F}, // 'U' 85
    {0x1F,0x20,0x40,0x20,0x1F}, // 'V' 86
    {0x3F,0x40,0x38,0x40,0x3F}, // 'W' 87
    {0x63,0x14,0x08,0x14,0x63}, // 'X' 88
    {0x07,0x08,0x70,0x08,0x07}, // 'Y' 89
    {0x61,0x51,0x49,0x45,0x43}, // 'Z' 90
    {0x00,0x7F,0x41,0x41,0x00}, // '[' 91
    {0x02,0x04,0x08,0x10,0x20}, // '\' 92
    {0x00,0x41,0x41,0x7F,0x00}, // ']' 93
    {0x04,0x02,0x01,0x02,0x04}, // '^' 94
    {0x40,0x40,0x40,0x40,0x40}, // '_' 95
    {0x00,0x01,0x02,0x04,0x00}, // '`' 96
    {0x20,0x54,0x54,0x54,0x78}, // 'a' 97
    {0x7F,0x48,0x44,0x44,0x38}, // 'b' 98
    {0x38,0x44,0x44,0x44,0x20}, // 'c' 99
    {0x38,0x44,0x44,0x48,0x7F}, // 'd' 100
    {0x38,0x54,0x54,0x54,0x18}, // 'e' 101
    {0x08,0x7E,0x09,0x01,0x02}, // 'f' 102
    {0x0C,0x52,0x52,0x52,0x3E}, // 'g' 103
    {0x7F,0x08,0x04,0x04,0x78}, // 'h' 104
    {0x00,0x44,0x7D,0x40,0x00}, // 'i' 105
    {0x20,0x40,0x44,0x3D,0x00}, // 'j' 106
    {0x7F,0x10,0x28,0x44,0x00}, // 'k' 107
    {0x00,0x41,0x7F,0x40,0x00}, // 'l' 108
    {0x7C,0x04,0x18,0x04,0x78}, // 'm' 109
    {0x7C,0x08,0x04,0x04,0x78}, // 'n' 110
    {0x38,0x44,0x44,0x44,0x38}, // 'o' 111
    {0x7C,0x14,0x14,0x14,0x08}, // 'p' 112
    {0x08,0x14,0x14,0x18,0x7C}, // 'q' 113
    {0x7C,0x08,0x04,0x04,0x08}, // 'r' 114
    {0x48,0x54,0x54,0x54,0x20}, // 's' 115
    {0x04,0x3F,0x44,0x40,0x20}, // 't' 116
    {0x3C,0x40,0x40,0x20,0x7C}, // 'u' 117
    {0x1C,0x20,0x40,0x20,0x1C}, // 'v' 118
    {0x3C,0x40,0x30,0x40,0x3C}, // 'w' 119
    {0x44,0x28,0x10,0x28,0x44}, // 'x' 120
    {0x0C,0x50,0x50,0x50,0x3C}, // 'y' 121
    {0x44,0x64,0x54,0x4C,0x44}, // 'z' 122
    {0x00,0x08,0x36,0x41,0x00}, // '{' 123
    {0x00,0x00,0x7F,0x00,0x00}, // '|' 124
    {0x00,0x41,0x36,0x08,0x00}, // '}' 125
    {0x10,0x08,0x08,0x10,0x08}, // '~' 126
};

void display_draw_char(int x, int y, char c, uint16_t color, uint16_t bg, uint8_t size)
{
    if (c < 32 || c > 126) c = '?';
    const uint8_t *glyph = font5x8[c - 32];
    for (int col = 0; col < 5; col++) {
        uint8_t line = glyph[col];
        for (int row = 0; row < 8; row++) {
            uint16_t pixel_color = (line & 0x01) ? color : bg;
            if (size == 1) {
                display_draw_pixel(x + col, y + row, pixel_color);
            } else {
                display_fill_rect(x + col * size, y + row * size, size, size, pixel_color);
            }
            line >>= 1;
        }
    }
    // 1px gap between chars
    if (bg != color) {
        display_fill_rect(x + 5 * size, y, size, 8 * size, bg);
    }
}

void display_draw_string(int x, int y, const char *str, uint16_t color, uint16_t bg, uint8_t size)
{
    while (*str) {
        display_draw_char(x, y, *str, color, bg, size);
        x += 6 * size;
        str++;
    }
}

uint16_t display_color565(uint8_t r, uint8_t g, uint8_t b)
{
    return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
}
