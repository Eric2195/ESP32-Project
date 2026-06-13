/*
 * gfx.c — Lightweight graphics primitives (framebuffer-based)
 */
#include "gfx.h"
#include "font.h"
#include <string.h>
#include <stdlib.h>
#include <math.h>

/* ── Internal helpers ───────────────────────────────────────── */

static inline void swap_int(int *a, int *b) { int t = *a; *a = *b; *b = t; }

/* ── Fill screen ────────────────────────────────────────────── */

void gfx_fill_screen(uint16_t color)
{
    uint16_t *fb = display_get_fb();
    if (!fb) return;
    /* Fast fill using 32-bit writes */
    uint32_t c32 = (uint32_t)color << 16 | color;
    uint32_t *fb32 = (uint32_t *)fb;
    for (int i = 0; i < (DISP_W * DISP_H) / 2; i++) {
        fb32[i] = c32;
    }
}

/* ── Pixel ──────────────────────────────────────────────────── */

void gfx_draw_pixel(int x, int y, uint16_t color)
{
    display_pixel(x, y, color);
}

/* ── Lines ──────────────────────────────────────────────────── */

void gfx_draw_fast_hline(int x, int y, int w, uint16_t color)
{
    uint16_t *fb = display_get_fb();
    if (!fb || y < 0 || y >= DISP_H) return;
    if (x < 0) { w += x; x = 0; }
    if (x + w > DISP_W) w = DISP_W - x;
    if (w <= 0) return;
    for (int i = 0; i < w; i++) {
        fb[y * DISP_W + x + i] = color;
    }
}

void gfx_draw_fast_vline(int x, int y, int h, uint16_t color)
{
    uint16_t *fb = display_get_fb();
    if (!fb || x < 0 || x >= DISP_W) return;
    if (y < 0) { h += y; y = 0; }
    if (y + h > DISP_H) h = DISP_H - y;
    if (h <= 0) return;
    for (int i = 0; i < h; i++) {
        fb[(y + i) * DISP_W + x] = color;
    }
}

void gfx_draw_line(int x0, int y0, int x1, int y1, uint16_t color)
{
    /* Bresenham's line algorithm */
    int dx = abs(x1 - x0);
    int dy = abs(y1 - y0);
    int sx = (x0 < x1) ? 1 : -1;
    int sy = (y0 < y1) ? 1 : -1;
    int err = dx - dy;

    while (1) {
        gfx_draw_pixel(x0, y0, color);
        if (x0 == x1 && y0 == y1) break;
        int e2 = 2 * err;
        if (e2 > -dy) { err -= dy; x0 += sx; }
        if (e2 < dx)  { err += dx; y0 += sy; }
    }
}

/* ── Rectangles ─────────────────────────────────────────────── */

void gfx_draw_rect(int x, int y, int w, int h, uint16_t color)
{
    gfx_draw_fast_hline(x, y, w, color);
    gfx_draw_fast_hline(x, y + h - 1, w, color);
    gfx_draw_fast_vline(x, y, h, color);
    gfx_draw_fast_vline(x + w - 1, y, h, color);
}

void gfx_fill_rect(int x, int y, int w, int h, uint16_t color)
{
    for (int i = 0; i < h; i++) {
        gfx_draw_fast_hline(x, y + i, w, color);
    }
}

/* ── Circles ────────────────────────────────────────────────── */

void gfx_draw_circle(int cx, int cy, int r, uint16_t color)
{
    int x = 0, y = r;
    int d = 3 - 2 * r;
    while (y >= x) {
        gfx_draw_pixel(cx + x, cy + y, color);
        gfx_draw_pixel(cx - x, cy + y, color);
        gfx_draw_pixel(cx + x, cy - y, color);
        gfx_draw_pixel(cx - x, cy - y, color);
        gfx_draw_pixel(cx + y, cy + x, color);
        gfx_draw_pixel(cx - y, cy + x, color);
        gfx_draw_pixel(cx + y, cy - x, color);
        gfx_draw_pixel(cx - y, cy - x, color);
        x++;
        if (d > 0) { y--; d += 4 * (x - y) + 10; }
        else       { d += 4 * x + 6; }
    }
}

void gfx_fill_circle(int cx, int cy, int r, uint16_t color)
{
    for (int y = -r; y <= r; y++) {
        for (int x = -r; x <= r; x++) {
            if (x * x + y * y <= r * r) {
                gfx_draw_pixel(cx + x, cy + y, color);
            }
        }
    }
}

/* ── Triangles ──────────────────────────────────────────────── */

static void fill_flat_bottom_triangle(int x0, int y0, int x1, int y1, int x2, int y2, uint16_t color)
{
    float inv_slope1 = (float)(x1 - x0) / (float)(y1 - y0);
    float inv_slope2 = (float)(x2 - x0) / (float)(y2 - y0);
    float cx1 = x0, cx2 = x0;
    for (int scan = y0; scan <= y1; scan++) {
        gfx_draw_fast_hline((int)cx1, scan, (int)cx2 - (int)cx1 + 1, color);
        cx1 += inv_slope1;
        cx2 += inv_slope2;
    }
}

static void fill_flat_top_triangle(int x0, int y0, int x1, int y1, int x2, int y2, uint16_t color)
{
    float inv_slope1 = (float)(x2 - x0) / (float)(y2 - y0);
    float inv_slope2 = (float)(x2 - x1) / (float)(y2 - y1);
    float cx1 = x2, cx2 = x2;
    for (int scan = y2; scan > y0; scan--) {
        gfx_draw_fast_hline((int)cx1, scan, (int)cx2 - (int)cx1 + 1, color);
        cx1 -= inv_slope1;
        cx2 -= inv_slope2;
    }
}

void gfx_fill_triangle(int x0, int y0, int x1, int y1, int x2, int y2, uint16_t color)
{
    /* Sort by Y */
    if (y0 > y1) { int t; t=x0;x0=x1;x1=t; t=y0;y0=y1;y1=t; }
    if (y1 > y2) { int t; t=x1;x1=x2;x2=t; t=y1;y1=y2;y2=t; }
    if (y0 > y1) { int t; t=x0;x0=x1;x1=t; t=y0;y0=y1;y1=t; }

    if (y1 == y2) {
        fill_flat_bottom_triangle(x0, y0, x1, y1, x2, y2, color);
    } else if (y0 == y1) {
        fill_flat_top_triangle(x0, y0, x1, y1, x2, y2, color);
    } else {
        int x3 = x0 + (int)((float)(y1 - y0) / (float)(y2 - y0) * (x2 - x0));
        fill_flat_bottom_triangle(x0, y0, x1, y1, x3, y1, color);
        fill_flat_top_triangle(x1, y1, x3, y1, x2, y2, color);
    }
}

void gfx_draw_triangle(int x0, int y0, int x1, int y1, int x2, int y2, uint16_t color)
{
    gfx_draw_line(x0, y0, x1, y1, color);
    gfx_draw_line(x1, y1, x2, y2, color);
    gfx_draw_line(x2, y2, x0, y0, color);
}

/* ── Text rendering ─────────────────────────────────────────── */

void gfx_draw_char(int x, int y, char c, uint16_t color, uint16_t bg, uint8_t size)
{
    if (c < 0x20 || c > 0x7E) c = '?';
    const uint8_t *glyph = &font_data[(c - 0x20) * FONT_HEIGHT];

    for (int row = 0; row < FONT_HEIGHT; row++) {
        uint8_t bits = glyph[row];
        for (int col = 0; col < FONT_WIDTH; col++) {
            uint16_t pc = (bits & (0x80 >> col)) ? color : bg;
            if (size == 1) {
                gfx_draw_pixel(x + col, y + row, pc);
            } else {
                gfx_fill_rect(x + col * size, y + row * size, size, size, pc);
            }
        }
    }
}

void gfx_draw_string(int x, int y, const char *s, uint16_t color, uint16_t bg, uint8_t size)
{
    while (*s) {
        gfx_draw_char(x, y, *s, color, bg, size);
        x += FONT_WIDTH * size;
        s++;
    }
}

int gfx_draw_string_adv(int x, int y, const char *s, uint16_t color, uint16_t bg, uint8_t size)
{
    while (*s) {
        gfx_draw_char(x, y, *s, color, bg, size);
        x += FONT_WIDTH * size;
        s++;
    }
    return x;
}
