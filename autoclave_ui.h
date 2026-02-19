#pragma once

#include "lvgl.h"

/* ============================================================
 * Autoclave Control System - LVGL UI
 * Target: ESP32-P4, 720x720 4" IPS MIPI-DSI
 * Theme: Material Dark (Android-inspired)
 * ============================================================ */

// ─── Screens ─────────────────────────────────────────────────
void ui_home_screen_init(void);
void ui_monitor_screen_init(void);
void ui_programs_screen_init(void);
void ui_settings_screen_init(void);
void ui_init(void);

// ─── Navigation ──────────────────────────────────────────────
void ui_navigate_to(int screen_index);

// ─── Live Data Update API ────────────────────────────────────
void ui_update_temperature(float temp_c);
void ui_update_pressure(float bar);
void ui_update_ssr_state(bool active);
void ui_update_status(const char *status_text);
void ui_add_log_entry(const char *msg);

// ─── Colour Palette (Material Dark) ─────────────────────────
#define COLOR_BG_BASE        lv_color_hex(0x121212)   // Screen background
#define COLOR_BG_SURFACE     lv_color_hex(0x1E1E1E)   // Card / surface
#define COLOR_BG_ELEVATED    lv_color_hex(0x2C2C2C)   // Elevated card
#define COLOR_PRIMARY        lv_color_hex(0x00BCD4)   // Cyan accent
#define COLOR_SECONDARY      lv_color_hex(0x26C6DA)   // Lighter cyan
#define COLOR_ACCENT_WARM    lv_color_hex(0xFF7043)   // Orange (heat/warning)
#define COLOR_ACCENT_GREEN   lv_color_hex(0x66BB6A)   // Green (OK / safe)
#define COLOR_ACCENT_RED     lv_color_hex(0xEF5350)   // Red (error / danger)
#define COLOR_ACCENT_YELLOW  lv_color_hex(0xFFA726)   // Amber (caution)
#define COLOR_TEXT_PRIMARY   lv_color_hex(0xFFFFFF)   // Primary text
#define COLOR_TEXT_SECONDARY lv_color_hex(0xB0BEC5)   // Secondary text
#define COLOR_TEXT_DISABLED  lv_color_hex(0x546E7A)   // Disabled text
#define COLOR_DIVIDER        lv_color_hex(0x263238)   // Divider / border
#define COLOR_NAVBAR         lv_color_hex(0x0D1117)   // Bottom nav bar

// ─── Dimensions ──────────────────────────────────────────────
#define SCREEN_W             720
#define SCREEN_H             720
#define NAVBAR_H             72
#define CONTENT_H            (SCREEN_H - NAVBAR_H)
#define CARD_RADIUS          16
#define BTN_RADIUS           12
#define PADDING_SM           8
#define PADDING_MD           16
#define PADDING_LG           24

// ─── Shared state (extern) ───────────────────────────────────
extern lv_obj_t *g_screen_home;
extern lv_obj_t *g_screen_monitor;
extern lv_obj_t *g_screen_programs;
extern lv_obj_t *g_screen_settings;

// ─── Home screen widgets (for live update) ───────────────────
extern lv_obj_t *g_arc_temp;
extern lv_obj_t *g_lbl_temp_value;
extern lv_obj_t *g_lbl_pressure_value;
extern lv_obj_t *g_lbl_status;
extern lv_obj_t *g_btn_ssr;
extern lv_obj_t *g_lbl_ssr;
extern lv_obj_t *g_chart_temp;
extern lv_chart_series_t *g_ser_temp;
