/*
 * CLAWD MOCHI — ESP32-C6 + ST7789 1.54" 240×240
 * Ported from Arduino/ESP32-C3 to ESP-IDF/ESP32-C6
 *
 * WiFi: "ClaWD-Mochi"  pw: clawd1234  → http://192.168.4.1
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_err.h"
#include "nvs_flash.h"

#include "display.h"
#include "wifi_ap.h"
#include "http_server.h"
#include "driver/usb_serial_jtag.h"

static const char *TAG = "clawd_mochi";

// ── Eye constants ────────────────────────────────────────────
#define EYE_W   30
#define EYE_H   60
#define EYE_GAP 120
#define EYE_OX  0
#define EYE_OY  40

// ── View IDs ─────────────────────────────────────────────────
#define VIEW_EYES_NORMAL 0
#define VIEW_EYES_SQUISH 1
#define VIEW_CODE        2
#define VIEW_DRAW        3

// ── Terminal constants ───────────────────────────────────────
#define TERM_COLS      15
#define TERM_ROWS       8
#define TERM_CHAR_W    12
#define TERM_CHAR_H    20
#define TERM_PAD_X      8
#define TERM_PAD_Y     18
#define PREFIX_PX      54   // 9 chars * 6px

// ── Logo constants ───────────────────────────────────────────
#define LOGO_CX 120
#define LOGO_CY 105
#define LOGO_TRI_COUNT 162
#define LOGO_SEG_COUNT 162

// ── Colours ──────────────────────────────────────────────────
static uint16_t C_ORANGE, C_DARKBG, C_MUTED, C_GREEN;
#define C_WHITE 0xFFFF
#define C_BLACK 0x0000

// ── State ────────────────────────────────────────────────────
static volatile uint8_t current_view = VIEW_EYES_NORMAL;
static volatile bool    busy_flag    = false;
static volatile bool    backlight_on = true;
static volatile uint8_t anim_speed   = 1;

static uint16_t anim_bg_color = 0;
static uint16_t draw_bg_color = 0;

// ── Terminal state ───────────────────────────────────────────
static volatile bool term_mode = false;
static char term_lines[TERM_ROWS][TERM_COLS + 1];
static uint8_t term_row = 0;
static uint8_t term_col = 0;

// ═════════════════════════════════════════════════════════════
//  LOGO DATA
// ═════════════════════════════════════════════════════════════

static const int16_t LOGO_TRIS[LOGO_TRI_COUNT][6] = {
  {120,105,65,134,100,114},{120,105,100,114,101,113},{120,105,101,113,100,112},
  {120,105,100,112,99,112},{120,105,99,112,93,111},{120,105,93,111,73,111},
  {120,105,73,111,55,110},{120,105,55,110,38,109},{120,105,38,109,34,108},
  {120,105,34,108,30,103},{120,105,30,103,30,100},{120,105,30,100,34,98},
  {120,105,34,98,39,98},{120,105,39,98,50,99},{120,105,50,99,67,100},
  {120,105,67,100,80,101},{120,105,80,101,98,103},{120,105,98,103,101,103},
  {120,105,101,103,101,102},{120,105,101,102,100,101},{120,105,100,101,100,100},
  {120,105,100,100,82,88},{120,105,82,88,63,76},{120,105,63,76,53,69},
  {120,105,53,69,48,65},{120,105,48,65,45,61},{120,105,45,61,44,54},
  {120,105,44,54,49,49},{120,105,49,49,55,49},{120,105,55,49,57,49},
  {120,105,57,49,64,55},{120,105,64,55,78,66},{120,105,78,66,96,79},
  {120,105,96,79,99,81},{120,105,99,81,100,81},{120,105,100,81,100,80},
  {120,105,100,80,99,78},{120,105,99,78,89,60},{120,105,89,60,78,41},
  {120,105,78,41,73,34},{120,105,73,34,72,29},{120,105,72,29,72,28},
  {120,105,72,28,72,27},{120,105,72,27,71,26},{120,105,71,26,71,25},
  {120,105,71,25,71,24},{120,105,71,24,77,16},{120,105,77,16,80,15},
  {120,105,80,15,87,16},{120,105,87,16,91,19},{120,105,91,19,95,29},
  {120,105,95,29,103,46},{120,105,103,46,114,68},{120,105,114,68,118,75},
  {120,105,118,75,119,81},{120,105,119,81,120,83},{120,105,120,83,121,83},
  {120,105,121,83,121,82},{120,105,121,82,122,69},{120,105,122,69,124,54},
  {120,105,124,54,126,34},{120,105,126,34,126,28},{120,105,126,28,129,21},
  {120,105,129,21,135,18},{120,105,135,18,139,20},{120,105,139,20,143,25},
  {120,105,143,25,142,28},{120,105,142,28,140,42},{120,105,140,42,136,64},
  {120,105,136,64,133,78},{120,105,133,78,135,78},{120,105,135,78,136,76},
  {120,105,136,76,144,67},{120,105,144,67,156,51},{120,105,156,51,162,45},
  {120,105,162,45,168,38},{120,105,168,38,172,35},{120,105,172,35,180,35},
  {120,105,180,35,185,43},{120,105,185,43,183,52},{120,105,183,52,175,62},
  {120,105,175,62,168,71},{120,105,168,71,159,83},{120,105,159,83,153,94},
  {120,105,153,94,154,94},{120,105,154,94,155,94},{120,105,155,94,176,90},
  {120,105,176,90,188,88},{120,105,188,88,201,85},{120,105,201,85,208,88},
  {120,105,208,88,208,91},{120,105,208,91,206,97},{120,105,206,97,191,101},
  {120,105,191,101,174,104},{120,105,174,104,148,110},{120,105,148,110,148,111},
  {120,105,148,111,148,111},{120,105,148,111,160,112},{120,105,160,112,165,112},
  {120,105,165,112,177,112},{120,105,177,112,200,114},{120,105,200,114,205,118},
  {120,105,205,118,209,123},{120,105,209,123,208,126},{120,105,208,126,199,131},
  {120,105,199,131,187,128},{120,105,187,128,159,121},{120,105,159,121,149,119},
  {120,105,149,119,147,119},{120,105,147,119,147,120},{120,105,147,120,156,128},
  {120,105,156,128,170,141},{120,105,170,141,189,158},{120,105,189,158,190,163},
  {120,105,190,163,188,166},{120,105,188,166,185,166},{120,105,185,166,169,153},
  {120,105,169,153,162,148},{120,105,162,148,148,136},{120,105,148,136,147,136},
  {120,105,147,136,147,137},{120,105,147,137,150,142},{120,105,150,142,168,168},
  {120,105,168,168,169,176},{120,105,169,176,168,179},{120,105,168,179,163,180},
  {120,105,163,180,158,179},{120,105,158,179,148,165},{120,105,148,165,137,149},
  {120,105,137,149,129,134},{120,105,129,134,128,135},{120,105,128,135,123,189},
  {120,105,123,189,120,192},{120,105,120,192,115,194},{120,105,115,194,110,191},
  {120,105,110,191,108,185},{120,105,108,185,110,174},{120,105,110,174,113,160},
  {120,105,113,160,116,148},{120,105,116,148,118,134},{120,105,118,134,119,129},
  {120,105,119,129,119,129},{120,105,119,129,118,129},{120,105,118,129,107,144},
  {120,105,107,144,91,166},{120,105,91,166,78,180},{120,105,78,180,75,181},
  {120,105,75,181,70,178},{120,105,70,178,70,173},{120,105,70,173,73,169},
  {120,105,73,169,91,146},{120,105,91,146,102,132},{120,105,102,132,109,124},
  {120,105,109,124,109,123},{120,105,109,123,108,123},{120,105,108,123,61,153},
  {120,105,61,153,52,155},{120,105,52,155,49,151},{120,105,49,151,49,146},
  {120,105,49,146,51,144},{120,105,51,144,65,134},{120,105,65,134,65,134},
};

static const int16_t LOGO_SEGS[LOGO_SEG_COUNT][4] = {
  {65,134,100,114},{100,114,101,113},{101,113,100,112},{100,112,99,112},
  {99,112,93,111},{93,111,73,111},{73,111,55,110},{55,110,38,109},
  {38,109,34,108},{34,108,30,103},{30,103,30,100},{30,100,34,98},
  {34,98,39,98},{39,98,50,99},{50,99,67,100},{67,100,80,101},
  {80,101,98,103},{98,103,101,103},{101,103,101,102},{101,102,100,101},
  {100,101,100,100},{100,100,82,88},{82,88,63,76},{63,76,53,69},
  {53,69,48,65},{48,65,45,61},{45,61,44,54},{44,54,49,49},
  {49,49,55,49},{55,49,57,49},{57,49,64,55},{64,55,78,66},
  {78,66,96,79},{96,79,99,81},{99,81,100,81},{100,81,100,80},
  {100,80,99,78},{99,78,89,60},{89,60,78,41},{78,41,73,34},
  {73,34,72,29},{72,29,72,28},{72,28,72,27},{72,27,71,26},
  {71,26,71,25},{71,25,71,24},{71,24,77,16},{77,16,80,15},
  {80,15,87,16},{87,16,91,19},{91,19,95,29},{95,29,103,46},
  {103,46,114,68},{114,68,118,75},{118,75,119,81},{119,81,120,83},
  {120,83,121,83},{121,83,121,82},{121,82,122,69},{122,69,124,54},
  {124,54,126,34},{126,34,126,28},{126,28,129,21},{129,21,135,18},
  {135,18,139,20},{139,20,143,25},{143,25,142,28},{142,28,140,42},
  {140,42,136,64},{136,64,133,78},{133,78,135,78},{135,78,136,76},
  {136,76,144,67},{144,67,156,51},{156,51,162,45},{162,45,168,38},
  {168,38,172,35},{172,35,180,35},{180,35,185,43},{185,43,183,52},
  {183,52,175,62},{175,62,168,71},{168,71,159,83},{159,83,153,94},
  {153,94,154,94},{154,94,155,94},{155,94,176,90},{176,90,188,88},
  {188,88,201,85},{201,85,208,88},{208,88,208,91},{208,91,206,97},
  {206,97,191,101},{191,101,174,104},{174,104,148,110},{148,110,148,111},
  {148,111,148,111},{148,111,160,112},{160,112,165,112},{165,112,177,112},
  {177,112,200,114},{200,114,205,118},{205,118,209,123},{209,123,208,126},
  {208,126,199,131},{199,131,187,128},{187,128,159,121},{159,121,149,119},
  {149,119,147,119},{147,119,147,120},{147,120,156,128},{156,128,170,141},
  {170,141,189,158},{189,158,190,163},{190,163,188,166},{188,166,185,166},
  {185,166,169,153},{169,153,162,148},{162,148,148,136},{148,136,147,136},
  {147,136,147,137},{147,137,150,142},{150,142,168,168},{168,168,169,176},
  {169,176,168,179},{168,179,163,180},{163,180,158,179},{158,179,148,165},
  {148,165,137,149},{137,149,129,134},{129,134,128,135},{128,135,123,189},
  {123,189,120,192},{120,192,115,194},{115,194,110,191},{110,191,108,185},
  {108,185,110,174},{110,174,113,160},{113,160,116,148},{116,148,118,134},
  {118,134,119,129},{119,129,119,129},{119,129,118,129},{118,129,107,144},
  {107,144,91,166},{91,166,78,180},{78,180,75,181},{75,181,70,178},
  {70,178,70,173},{70,173,73,169},{73,169,91,146},{91,146,102,132},
  {102,132,109,124},{109,124,109,123},{109,123,108,123},{108,123,61,153},
  {61,153,52,155},{52,155,49,151},{49,151,49,146},{49,146,51,144},
  {51,144,65,134},{65,134,65,134},
};

// ═════════════════════════════════════════════════════════════
//  HELPERS
// ═════════════════════════════════════════════════════════════

static int speed_ms(int ms) {
    if (anim_speed == 3) return ms / 2;
    if (anim_speed == 1) return ms * 2;
    return ms;
}

static uint16_t hex_to_rgb565(const char *hex) {
    if (!hex) return C_WHITE;
    if (hex[0] == '#') hex++;
    if (strlen(hex) != 6) return C_WHITE;
    char buf[7];
    memcpy(buf, hex, 6);
    buf[6] = 0;
    long v = strtol(buf, NULL, 16);
    return display_color565((v >> 16) & 0xFF, (v >> 8) & 0xFF, v & 0xFF);
}

static void rgb565_to_hex(uint16_t c, char *buf, size_t len) {
    uint8_t r = ((c >> 11) & 0x1F) << 3;
    uint8_t g = ((c >> 5)  & 0x3F) << 2;
    uint8_t b = (c & 0x1F) << 3;
    snprintf(buf, len, "#%02x%02x%02x", r, g, b);
}

static void init_colours(void) {
    C_ORANGE = display_color565(255, 120, 40);
    C_DARKBG = display_color565(10, 12, 16);
    C_MUTED  = display_color565(90, 88, 86);
    C_GREEN  = display_color565(80, 220, 130);
    anim_bg_color = C_ORANGE;
    draw_bg_color = C_ORANGE;
}

// ═════════════════════════════════════════════════════════════
//  EYE HELPERS
// ═════════════════════════════════════════════════════════════

static inline int16_t eye_lx(int16_t ox) {
    return (DISP_W - (EYE_W * 2 + EYE_GAP)) / 2 + EYE_OX + ox;
}
static inline int16_t eye_rx(int16_t ox) { return eye_lx(ox) + EYE_W + EYE_GAP; }
static inline int16_t eye_y(void)       { return (DISP_H - EYE_H) / 2 - EYE_OY; }
static inline int16_t eye_cy(void)      { return eye_y() + EYE_H / 2; }

// ═════════════════════════════════════════════════════════════
//  DRAWING FUNCTIONS
// ═════════════════════════════════════════════════════════════

static void draw_logo_filled(uint16_t bg, uint16_t fg) {
    display_fill_screen(bg);
    for (int i = 0; i < LOGO_TRI_COUNT; i++) {
        display_fill_triangle(
            LOGO_TRIS[i][0], LOGO_TRIS[i][1],
            LOGO_TRIS[i][2], LOGO_TRIS[i][3],
            LOGO_TRIS[i][4], LOGO_TRIS[i][5],
            fg);
    }
    display_draw_string(LOGO_CX - 54, 210, "Anthropic", fg, bg, 2);
}

static void draw_normal_eyes(int16_t ox, bool blink) {
    display_fill_screen(C_ORANGE);
    int16_t lx = eye_lx(ox), rx = eye_rx(ox), ey = eye_y();
    if (!blink) {
        display_fill_rect(lx, ey, EYE_W, EYE_H, C_BLACK);
        display_fill_rect(rx, ey, EYE_W, EYE_H, C_BLACK);
    } else {
        display_fill_rect(lx, ey + EYE_H / 2 - 3, EYE_W, 6, C_BLACK);
        display_fill_rect(rx, ey + EYE_H / 2 - 3, EYE_W, 6, C_BLACK);
    }
}

static void draw_chevron(int16_t cx, int16_t cy, int16_t arm, int16_t reach,
                          uint8_t thk, bool right_facing, uint16_t col) {
    for (int8_t t = -(int8_t)thk; t <= (int8_t)thk; t++) {
        if (right_facing) {
            display_draw_line(cx - reach/2, cy - arm + t, cx + reach/2, cy + t,      col);
            display_draw_line(cx + reach/2, cy + t,       cx - reach/2, cy + arm + t, col);
        } else {
            display_draw_line(cx + reach/2, cy - arm + t, cx - reach/2, cy + t,      col);
            display_draw_line(cx - reach/2, cy + t,       cx + reach/2, cy + arm + t, col);
        }
    }
}

static void draw_squish_eyes(bool closed) {
    display_fill_screen(anim_bg_color);
    int16_t lx = eye_lx(0), rx = eye_rx(0), cy = eye_cy();
    int16_t arm   = EYE_H / 2;
    int16_t reach = EYE_W / 2;
    int16_t lcx   = lx + EYE_W / 2;
    int16_t rcx   = rx + EYE_W / 2;
    if (!closed) {
        draw_chevron(lcx, cy, arm, reach, 10, true,  C_BLACK);
        draw_chevron(rcx, cy, arm, reach, 10, false, C_BLACK);
    } else {
        display_fill_rect(lx, cy - 5, EYE_W, 10, C_BLACK);
        display_fill_rect(rx, cy - 5, EYE_W, 10, C_BLACK);
    }
}

static void draw_code_view(void) {
    term_mode = false;
    display_fill_screen(C_DARKBG);
    display_fill_rect(0, 0,          DISP_W, 4, C_ORANGE);
    display_fill_rect(0, DISP_H - 4, DISP_W, 4, C_ORANGE);
    display_draw_string((DISP_W - 144) / 2, DISP_H / 2 - 52, "Claude", C_ORANGE, C_ORANGE, 4);
    display_draw_string((DISP_W - 96) / 2,  DISP_H / 2 + 8,  "Code",   C_WHITE,  C_WHITE,  4);
    display_fill_rect((DISP_W - 96) / 2, DISP_H / 2 + 52, 96, 3, C_ORANGE);
}

// ═════════════════════════════════════════════════════════════
//  TERMINAL FUNCTIONS
// ═════════════════════════════════════════════════════════════

static void term_clear(void) {
    for (uint8_t i = 0; i < TERM_ROWS; i++) term_lines[i][0] = '\0';
    term_row = 0; term_col = 0;
}

static void term_draw_header(void) {
    display_fill_rect(0, 0, DISP_W, TERM_PAD_Y + 1, C_DARKBG);
    display_draw_string(TERM_PAD_X, 4, "clawd@mochi terminal", C_ORANGE, C_DARKBG, 1);
    display_draw_fast_hline(0, TERM_PAD_Y, DISP_W, C_ORANGE);
}

static void term_draw_prefix(int16_t yy) {
    display_draw_string(TERM_PAD_X, yy + 6, "clawd:~$ ", C_GREEN, C_DARKBG, 1);
}

static void term_draw_line(uint8_t r) {
    int16_t yy = TERM_PAD_Y + 4 + r * TERM_CHAR_H;
    display_fill_rect(0, yy, DISP_W, TERM_CHAR_H, C_DARKBG);
    if (r == term_row) term_draw_prefix(yy);
    display_draw_string(TERM_PAD_X + PREFIX_PX, yy + 1, term_lines[r], C_WHITE, C_DARKBG, 2);
    if (r == term_row) {
        int16_t cx = TERM_PAD_X + PREFIX_PX + term_col * TERM_CHAR_W;
        display_fill_rect(cx, yy + 1, TERM_CHAR_W - 2, TERM_CHAR_H - 2, C_GREEN);
    }
}

static void term_draw_last_char(void) {
    if (term_col == 0) return;
    int16_t yy    = TERM_PAD_Y + 4 + term_row * TERM_CHAR_H;
    int16_t baseX = TERM_PAD_X + PREFIX_PX;
    uint8_t prev  = term_col - 1;
    // erase prev cell (had cursor block)
    display_fill_rect(baseX + prev * TERM_CHAR_W, yy + 1, TERM_CHAR_W, TERM_CHAR_H - 1, C_DARKBG);
    // draw the character
    char ch[2] = { term_lines[term_row][prev], '\0' };
    display_draw_string(baseX + prev * TERM_CHAR_W, yy + 1, ch, C_WHITE, C_DARKBG, 2);
    // new cursor
    display_fill_rect(baseX + term_col * TERM_CHAR_W, yy + 1, TERM_CHAR_W - 2, TERM_CHAR_H - 2, C_GREEN);
}

static void term_draw_backspace(void) {
    int16_t yy    = TERM_PAD_Y + 4 + term_row * TERM_CHAR_H;
    int16_t baseX = TERM_PAD_X + PREFIX_PX;
    display_fill_rect(baseX + term_col * TERM_CHAR_W, yy + 1, TERM_CHAR_W * 2, TERM_CHAR_H - 1, C_DARKBG);
    display_fill_rect(baseX + term_col * TERM_CHAR_W, yy + 1, TERM_CHAR_W - 2, TERM_CHAR_H - 2, C_GREEN);
    if (strlen(term_lines[term_row]) == 0) {
        display_fill_rect(0, yy, TERM_PAD_X + PREFIX_PX, TERM_CHAR_H, C_DARKBG);
    }
}

static void term_full_redraw(void) {
    display_fill_screen(C_DARKBG);
    term_draw_header();
    for (uint8_t r = 0; r < TERM_ROWS; r++) term_draw_line(r);
}

static void term_scroll(void) {
    for (uint8_t i = 0; i < TERM_ROWS - 1; i++) {
        strncpy(term_lines[i], term_lines[i + 1], TERM_COLS);
        term_lines[i][TERM_COLS] = '\0';
    }
    term_lines[TERM_ROWS - 1][0] = '\0';
    term_row = TERM_ROWS - 1;
    term_full_redraw();
}

static void term_add_char(char c) {
    if (c == '\n' || c == '\r') {
        int16_t yy = TERM_PAD_Y + 4 + term_row * TERM_CHAR_H;
        display_fill_rect(TERM_PAD_X + PREFIX_PX + term_col * TERM_CHAR_W,
                          yy + 1, TERM_CHAR_W, TERM_CHAR_H - 1, C_DARKBG);
        term_row++; term_col = 0;
        if (term_row >= TERM_ROWS) { term_scroll(); return; }
        term_draw_line(term_row);
    } else if (c == '\b' || c == 127) {
        if (term_col > 0) {
            term_col--;
            term_lines[term_row][term_col] = '\0';
            term_draw_backspace();
        }
    } else if (c >= 32 && c < 127) {
        if (term_col >= TERM_COLS) {
            term_row++; term_col = 0;
            if (term_row >= TERM_ROWS) { term_scroll(); return; }
        }
        if (term_col == 0) term_draw_prefix(TERM_PAD_Y + 4 + term_row * TERM_CHAR_H);
        term_lines[term_row][term_col] = c;
        term_col++;
        term_lines[term_row][term_col] = '\0';
        term_draw_last_char();
    }
}

// ═════════════════════════════════════════════════════════════
//  ANIMATIONS
// ═════════════════════════════════════════════════════════════

static void anim_normal_eyes(void *arg) {
    busy_flag = true;
    const int16_t offs[] = {-16, 16, -16, 16, 0};
    for (uint8_t i = 0; i < 5; i++) {
        draw_normal_eyes(offs[i], false);
        vTaskDelay(pdMS_TO_TICKS(speed_ms(80)));
    }
    draw_normal_eyes(0, true);  vTaskDelay(pdMS_TO_TICKS(speed_ms(100)));
    draw_normal_eyes(0, false); vTaskDelay(pdMS_TO_TICKS(speed_ms(70)));
    draw_normal_eyes(0, true);  vTaskDelay(pdMS_TO_TICKS(speed_ms(70)));
    draw_normal_eyes(0, false);
    busy_flag = false;
    vTaskDelete(NULL);
}

static void anim_squish_eyes(void *arg) {
    busy_flag = true;
    for (uint8_t i = 0; i < 3; i++) {
        draw_squish_eyes(false); vTaskDelay(pdMS_TO_TICKS(speed_ms(160)));
        draw_squish_eyes(true);  vTaskDelay(pdMS_TO_TICKS(speed_ms(100)));
    }
    draw_squish_eyes(false);
    busy_flag = false;
    vTaskDelete(NULL);
}

static void anim_logo_reveal(void *arg) {
    busy_flag = true;
    display_fill_screen(anim_bg_color);
    for (int i = 0; i < LOGO_SEG_COUNT; i++) {
        int16_t x1 = LOGO_SEGS[i][0], y1 = LOGO_SEGS[i][1];
        int16_t x2 = LOGO_SEGS[i][2], y2 = LOGO_SEGS[i][3];
        display_draw_line(x1, y1, x2, y2, C_ORANGE);
        display_draw_line(x1 + 1, y1, x2 + 1, y2, C_ORANGE);
        if (i % 4 == 0) vTaskDelay(pdMS_TO_TICKS(speed_ms(8)));
    }
    draw_logo_filled(C_DARKBG, C_ORANGE);
    vTaskDelay(pdMS_TO_TICKS(1500));
    busy_flag = false;
    vTaskDelete(NULL);
}

// ═════════════════════════════════════════════════════════════
//  HTTP CALLBACKS
// ═════════════════════════════════════════════════════════════

static void on_cmd(char key) {
    if (term_mode) {
        if (key == 'q') {
            term_mode = false;
            draw_code_view();
        }
        return;
    }
    switch (key) {
        case 'w':
            current_view = VIEW_EYES_NORMAL;
            xTaskCreate(anim_normal_eyes, "anim_eyes", 4096, NULL, 5, NULL);
            break;
        case 's':
            current_view = VIEW_EYES_SQUISH;
            xTaskCreate(anim_squish_eyes, "anim_squish", 4096, NULL, 5, NULL);
            break;
        case 'd':
            current_view = VIEW_CODE;
            draw_code_view();
            term_mode = true;
            term_clear();
            term_full_redraw();
            break;
        case 'a':
            current_view = VIEW_EYES_NORMAL;
            xTaskCreate(anim_logo_reveal, "anim_logo", 4096, NULL, 5, NULL);
            break;
    }
}

static void on_char(char c) {
    if (term_mode) term_add_char(c);
}

static void on_speed(int speed) {
    if (speed >= 1 && speed <= 3) anim_speed = speed;
}

static void on_backlight(bool on) {
    backlight_on = on;
    display_set_backlight(on);
}

static void on_redraw(const char *bg_hex) {
    if (bg_hex && bg_hex[0]) {
        anim_bg_color = hex_to_rgb565(bg_hex);
        draw_bg_color = anim_bg_color;
    }
    switch (current_view) {
        case VIEW_EYES_NORMAL: draw_normal_eyes(0, false); break;
        case VIEW_EYES_SQUISH: draw_squish_eyes(false);    break;
        case VIEW_CODE:        draw_code_view();           break;
        case VIEW_DRAW:        display_fill_screen(draw_bg_color); break;
    }
}

static void on_canvas(bool on) {
    if (on) {
        current_view = VIEW_DRAW;
        display_fill_screen(draw_bg_color);
    }
}

static void on_draw_clear(const char *bg_hex) {
    if (bg_hex && bg_hex[0]) {
        draw_bg_color = hex_to_rgb565(bg_hex);
        anim_bg_color = draw_bg_color;
    }
    current_view = VIEW_DRAW;
    term_mode = false;
    display_fill_screen(draw_bg_color);
}

static void on_draw_stroke(const char *pen_hex, const char *pts) {
    if (!pts || !pen_hex) return;
    uint16_t color = hex_to_rgb565(pen_hex);
    current_view = VIEW_DRAW;

    int16_t prev_x = -1, prev_y = -1;
    const char *p = pts;
    while (*p) {
        // parse "x,y;"
        int16_t x = (int16_t)strtol(p, (char **)&p, 10);
        if (*p == ',') p++;
        int16_t y = (int16_t)strtol(p, (char **)&p, 10);
        if (*p == ';') p++;

        if (prev_x >= 0) {
            display_draw_line(prev_x, prev_y, x, y, color);
            display_draw_line(prev_x + 1, prev_y, x + 1, y, color);
            display_draw_line(prev_x, prev_y + 1, x, y + 1, color);
        } else {
            display_fill_circle(x, y, 2, color);
        }
        prev_x = x; prev_y = y;
    }
}

// ═════════════════════════════════════════════════════════════
//  BOOT SPLASH & WIFI INFO SCREEN
// ═════════════════════════════════════════════════════════════

static void show_boot_splash(void) {
    display_fill_screen(C_ORANGE);
    display_draw_string(DISP_W / 2 - 54, DISP_H / 2 - 22, "Clawd", C_WHITE, C_ORANGE, 3);
    display_draw_string(DISP_W / 2 - 54, DISP_H / 2 + 14, "Mochi", C_WHITE, C_ORANGE, 3);
    vTaskDelay(pdMS_TO_TICKS(800));
}

static void show_wifi_info(void) {
    display_fill_screen(C_DARKBG);
    display_fill_rect(0, 0, DISP_W, 4, C_ORANGE);

    if (wifi_sta_is_connected()) {
        const char *ip = wifi_sta_get_ip();
        display_draw_string(12, 20,  "Connected!", C_GREEN, C_DARKBG, 2);
        display_draw_string(12, 50,  "Open browser:", C_WHITE, C_DARKBG, 2);
        char ip_line[32];
        snprintf(ip_line, sizeof(ip_line), "http://%s", ip);
        display_draw_string(12, 76,  ip_line, C_ORANGE, C_DARKBG, 2);
        display_draw_string(12, 110, "Claude Code hooks", C_GREEN, C_DARKBG, 1);
        display_draw_string(12, 124, "ready!", C_GREEN, C_DARKBG, 1);
    } else {
        display_draw_string(12, 30,  "Connecting to", C_WHITE, C_DARKBG, 2);
        display_draw_string(12, 56,  "iPhone 17", C_ORANGE, C_DARKBG, 2);
        display_draw_string(12, 90,  "Please wait...", C_MUTED, C_DARKBG, 1);
    }
}

// ═════════════════════════════════════════════════════════════
//  APP_MAIN
// ═════════════════════════════════════════════════════════════

void app_main(void) {
    // Init NVS (required by WiFi)
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // Init display
    ESP_ERROR_CHECK(display_init());
    display_set_backlight(true);
    init_colours();

    // Boot splash (brief)
    show_boot_splash();

    // Init WiFi AP+STA
    ESP_ERROR_CHECK(wifi_ap_init());

    // Wait briefly for STA to connect
    for (int i = 0; i < 6 && !wifi_sta_is_connected(); i++) {
        vTaskDelay(pdMS_TO_TICKS(500));
    }

    // Show WiFi info screen briefly
    show_wifi_info();
    vTaskDelay(pdMS_TO_TICKS(3000));

    // Switch to normal eyes
    draw_normal_eyes(0, false);
    current_view = VIEW_EYES_NORMAL;

    // Start HTTP server with callbacks
    http_server_config_t cfg = {
        .on_cmd        = on_cmd,
        .on_char       = on_char,
        .on_speed      = on_speed,
        .on_backlight  = on_backlight,
        .on_redraw     = on_redraw,
        .on_canvas     = on_canvas,
        .on_draw_clear = on_draw_clear,
        .on_draw_stroke = on_draw_stroke,
        .current_view  = &current_view,
        .busy          = &busy_flag,
        .term_mode     = &term_mode,
        .backlight_on  = &backlight_on,
        .anim_speed    = &anim_speed,
    };
    ESP_ERROR_CHECK(http_server_start(&cfg));

    // Init USB Serial JTAG for input
    usb_serial_jtag_driver_config_t usb_cfg = USB_SERIAL_JTAG_DRIVER_CONFIG_DEFAULT();
    usb_serial_jtag_driver_install(&usb_cfg);

    ESP_LOGI(TAG, "Clawd Mochi ready! Listening on USB serial...");

    // Serial command loop — reads from USB Serial JTAG
    while (1) {
        char c;
        int len = usb_serial_jtag_read_bytes(&c, 1, pdMS_TO_TICKS(100));
        if (len > 0) {
            switch (c) {
                case 'w': on_cmd('w'); break;  // normal eyes
                case 's': on_cmd('s'); break;  // squish eyes
                case 'd': on_cmd('d'); break;  // code view
                case 'a': on_cmd('a'); break;  // logo reveal
                case 'q': on_cmd('q'); break;  // quit
                case 'b':                     // toggle backlight
                    backlight_on = !backlight_on;
                    display_set_backlight(backlight_on);
                    break;
            }
        }
    }
}
