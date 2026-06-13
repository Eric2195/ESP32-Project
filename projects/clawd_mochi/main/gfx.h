/*
 * gfx.h — Lightweight graphics primitives (framebuffer-based)
 */
#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "display.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ── Drawing primitives ─────────────────────────────────────── */

void gfx_fill_screen(uint16_t color);

void gfx_draw_pixel(int x, int y, uint16_t color);

void gfx_draw_line(int x0, int y0, int x1, int y1, uint16_t color);

void gfx_draw_rect(int x, int y, int w, int h, uint16_t color);
void gfx_fill_rect(int x, int y, int w, int h, uint16_t color);

void gfx_draw_circle(int cx, int cy, int r, uint16_t color);
void gfx_fill_circle(int cx, int cy, int r, uint16_t color);

void gfx_draw_triangle(int x0, int y0, int x1, int y1, int x2, int y2, uint16_t color);
void gfx_fill_triangle(int x0, int y0, int x1, int y1, int x2, int y2, uint16_t color);

/* ── Text ───────────────────────────────────────────────────── */

/**
 * @brief Draw a character at (x,y) with given color and size multiplier
 *        Uses built-in 8x16 bitmap font. Size 1 = 8x16, Size 2 = 16x32, etc.
 */
void gfx_draw_char(int x, int y, char c, uint16_t color, uint16_t bg, uint8_t size);

/**
 * @brief Draw a string at (x,y)
 */
void gfx_draw_string(int x, int y, const char *s, uint16_t color, uint16_t bg, uint8_t size);

/**
 * @brief Draw a string and return the x position after the last char
 */
int gfx_draw_string_adv(int x, int y, const char *s, uint16_t color, uint16_t bg, uint8_t size);

/* ── Convenience ────────────────────────────────────────────── */

/* Horizontal line (fast) */
void gfx_draw_fast_hline(int x, int y, int w, uint16_t color);

/* Vertical line (fast) */
void gfx_draw_fast_vline(int x, int y, int h, uint16_t color);

#ifdef __cplusplus
}
#endif
