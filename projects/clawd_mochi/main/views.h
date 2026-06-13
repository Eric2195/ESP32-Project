/*
 * views.h — Eye animations, logo, terminal, and canvas views
 */
#pragma once

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ── View IDs ───────────────────────────────────────────────── */
#define VIEW_EYES_NORMAL 0
#define VIEW_EYES_SQUISH 1
#define VIEW_CODE        2
#define VIEW_DRAW        3

/* ── State (read-only from outside) ─────────────────────────── */
extern uint8_t  g_current_view;
extern bool     g_busy;
extern bool     g_backlight_on;
extern uint8_t  g_anim_speed;
extern uint16_t g_anim_bg_color;
extern uint16_t g_draw_bg_color;

/* ── Colors (initialized by views_init) ─────────────────────── */
extern uint16_t C_ORANGE;
extern uint16_t C_DARKBG;
extern uint16_t C_MUTED;
extern uint16_t C_GREEN;

/* ── API ────────────────────────────────────────────────────── */

/**
 * @brief Initialize colors and state. Call after display_init().
 */
void views_init(void);

/**
 * @brief Show boot splash (Clawd Mochi text + logo)
 */
void views_boot_splash(void);

/**
 * @brief Show WiFi info screen
 */
void views_show_wifi_info(void);

/**
 * @brief Trigger normal eyes animation (wiggle + blink)
 */
void views_anim_normal_eyes(void);

/**
 * @brief Trigger squish eyes animation (open/close)
 */
void views_anim_squish_eyes(void);

/**
 * @brief Show Claude Code view (title + enter terminal mode)
 */
void views_show_code(void);

/**
 * @brief Draw a single character in terminal mode
 */
void views_term_add_char(char c);

/**
 * @brief Exit terminal mode
 */
void views_term_exit(void);

/**
 * @brief Enter canvas (draw) mode
 */
void views_canvas_enter(void);

/**
 * @brief Clear canvas with given color
 */
void views_canvas_clear(uint16_t bg_color);

/**
 * @brief Draw a stroke on canvas (points separated by ';', format "x,y;x,y;...")
 */
void views_canvas_stroke(const char *pts, uint16_t pen_color);

/**
 * @brief Redraw current view with new background color
 */
void views_redraw(uint16_t bg_color);

/**
 * @brief Get animation speed delay in ms
 */
int views_speed_ms(int ms);

#ifdef __cplusplus
}
#endif
