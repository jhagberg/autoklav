/*
 * ============================================================
 *  Autoklav Control System — LVGL 9.x UI
 *  ESP32-P4 · 720×720 4" IPS MIPI-DSI
 *
 *  Skärmar:
 *    0 · Home     — Stor temperaturvisare, tryck, SSR-knapp
 *    1 · Monitor  — Realtidsdiagram, larmlogg
 *    2 · Program  — Steriliseringsprogram (valbara cykler)
 *    3 · Settings — PID, Nätverk, System
 *
 *  Tema: Material Dark (Android-inspirerat)
 * ============================================================
 */

#include "autoclave_ui.h"
#include <stdio.h>
#include <string.h>

// ─── Globals ─────────────────────────────────────────────────
lv_obj_t *g_screen_home;
lv_obj_t *g_screen_monitor;
lv_obj_t *g_screen_programs;
lv_obj_t *g_screen_settings;

lv_obj_t *g_arc_temp;
lv_obj_t *g_lbl_temp_value;
lv_obj_t *g_lbl_pressure_value;
lv_obj_t *g_lbl_status;
lv_obj_t *g_btn_ssr;
lv_obj_t *g_lbl_ssr;
lv_obj_t *g_chart_temp;
lv_chart_series_t *g_ser_temp;

// Navigation bar labels (active indicator)
static lv_obj_t *g_nav_indicators[4];
static int g_active_screen = 0;

// Monitor screen log
static lv_obj_t *g_log_list;

// Settings widgets
static lv_obj_t *g_slider_kp;
static lv_obj_t *g_slider_ki;
static lv_obj_t *g_slider_kd;
static lv_obj_t *g_lbl_kp_val;
static lv_obj_t *g_lbl_ki_val;
static lv_obj_t *g_lbl_kd_val;

// ─── Helper: make a card surface ─────────────────────────────
static lv_obj_t *make_card(lv_obj_t *parent, int x, int y, int w, int h)
{
    lv_obj_t *card = lv_obj_create(parent);
    lv_obj_set_pos(card, x, y);
    lv_obj_set_size(card, w, h);
    lv_obj_set_style_bg_color(card, COLOR_BG_SURFACE, 0);
    lv_obj_set_style_bg_opa(card, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(card, CARD_RADIUS, 0);
    lv_obj_set_style_border_width(card, 1, 0);
    lv_obj_set_style_border_color(card, COLOR_DIVIDER, 0);
    lv_obj_set_style_pad_all(card, PADDING_MD, 0);
    lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);
    return card;
}

// ─── Helper: title label in a card ───────────────────────────
static lv_obj_t *make_card_title(lv_obj_t *parent, const char *txt)
{
    lv_obj_t *lbl = lv_label_create(parent);
    lv_label_set_text(lbl, txt);
    lv_obj_set_style_text_color(lbl, COLOR_TEXT_SECONDARY, 0);
    lv_obj_set_style_text_font(lbl, &lv_font_montserrat_14, 0);
    lv_obj_align(lbl, LV_ALIGN_TOP_LEFT, 0, 0);
    return lbl;
}

// ─── Helper: value label ─────────────────────────────────────
static lv_obj_t *make_value_label(lv_obj_t *parent, const char *txt,
                                   const lv_font_t *font, lv_color_t color)
{
    lv_obj_t *lbl = lv_label_create(parent);
    lv_label_set_text(lbl, txt);
    lv_obj_set_style_text_color(lbl, color, 0);
    lv_obj_set_style_text_font(lbl, font, 0);
    return lbl;
}

// ─── Helper: filled button ───────────────────────────────────
static lv_obj_t *make_button(lv_obj_t *parent, const char *txt,
                               lv_color_t bg, int w, int h,
                               lv_event_cb_t cb)
{
    lv_obj_t *btn = lv_btn_create(parent);
    lv_obj_set_size(btn, w, h);
    lv_obj_set_style_bg_color(btn, bg, 0);
    lv_obj_set_style_bg_color(btn, lv_color_darken(bg, 40), LV_STATE_PRESSED);
    lv_obj_set_style_radius(btn, BTN_RADIUS, 0);
    lv_obj_set_style_shadow_width(btn, 8, 0);
    lv_obj_set_style_shadow_color(btn, bg, 0);
    lv_obj_set_style_shadow_opa(btn, LV_OPA_30, 0);
    lv_obj_set_style_border_width(btn, 0, 0);
    lv_obj_t *lbl = lv_label_create(btn);
    lv_label_set_text(lbl, txt);
    lv_obj_set_style_text_color(lbl, COLOR_TEXT_PRIMARY, 0);
    lv_obj_set_style_text_font(lbl, &lv_font_montserrat_16, 0);
    lv_obj_center(lbl);
    if (cb) lv_obj_add_event_cb(btn, cb, LV_EVENT_CLICKED, NULL);
    return btn;
}

// ═══════════════════════════════════════════════════════════════
//  BOTTOM NAVIGATION BAR  (shared across all screens)
// ═══════════════════════════════════════════════════════════════
typedef struct { const char *icon; const char *label; int idx; } NavItem;

static const NavItem NAV_ITEMS[] = {
    { LV_SYMBOL_HOME,     "Hem",       0 },
    { LV_SYMBOL_CHART,    "Monitor",   1 },
    { LV_SYMBOL_LIST,     "Program",   2 },
    { LV_SYMBOL_SETTINGS, "Inställn.", 3 },
};

static void nav_btn_cb(lv_event_t *e)
{
    int idx = (int)(intptr_t)lv_event_get_user_data(e);
    ui_navigate_to(idx);
}

static void create_navbar(lv_obj_t *screen, int active_idx)
{
    lv_obj_t *bar = lv_obj_create(screen);
    lv_obj_set_pos(bar, 0, CONTENT_H);
    lv_obj_set_size(bar, SCREEN_W, NAVBAR_H);
    lv_obj_set_style_bg_color(bar, COLOR_NAVBAR, 0);
    lv_obj_set_style_bg_opa(bar, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(bar, 1, 0);
    lv_obj_set_style_border_side(bar, LV_BORDER_SIDE_TOP, 0);
    lv_obj_set_style_border_color(bar, COLOR_DIVIDER, 0);
    lv_obj_set_style_radius(bar, 0, 0);
    lv_obj_set_style_pad_all(bar, 0, 0);
    lv_obj_clear_flag(bar, LV_OBJ_FLAG_SCROLLABLE);

    int btn_w = SCREEN_W / 4;
    for (int i = 0; i < 4; i++) {
        lv_obj_t *btn = lv_btn_create(bar);
        lv_obj_set_pos(btn, i * btn_w, 0);
        lv_obj_set_size(btn, btn_w, NAVBAR_H);
        lv_obj_set_style_bg_opa(btn, LV_OPA_TRANSP, 0);
        lv_obj_set_style_bg_opa(btn, LV_OPA_10, LV_STATE_PRESSED);
        lv_obj_set_style_bg_color(btn, COLOR_PRIMARY, LV_STATE_PRESSED);
        lv_obj_set_style_shadow_width(btn, 0, 0);
        lv_obj_set_style_border_width(btn, 0, 0);
        lv_obj_set_style_radius(btn, 0, 0);
        lv_obj_add_event_cb(btn, nav_btn_cb, LV_EVENT_CLICKED,
                            (void *)(intptr_t)NAV_ITEMS[i].idx);

        // Icon
        lv_obj_t *icon = lv_label_create(btn);
        lv_label_set_text(icon, NAV_ITEMS[i].icon);
        bool is_active = (i == active_idx);
        lv_obj_set_style_text_color(icon, is_active ? COLOR_PRIMARY : COLOR_TEXT_DISABLED, 0);
        lv_obj_set_style_text_font(icon, &lv_font_montserrat_20, 0);
        lv_obj_align(icon, LV_ALIGN_CENTER, 0, -8);

        // Label
        lv_obj_t *lbl = lv_label_create(btn);
        lv_label_set_text(lbl, NAV_ITEMS[i].label);
        lv_obj_set_style_text_color(lbl, is_active ? COLOR_PRIMARY : COLOR_TEXT_DISABLED, 0);
        lv_obj_set_style_text_font(lbl, &lv_font_montserrat_10, 0);
        lv_obj_align(lbl, LV_ALIGN_CENTER, 0, 16);

        // Active dot indicator
        if (is_active) {
            lv_obj_t *dot = lv_obj_create(bar);
            lv_obj_set_pos(dot, i * btn_w + btn_w / 2 - 16, 2);
            lv_obj_set_size(dot, 32, 3);
            lv_obj_set_style_bg_color(dot, COLOR_PRIMARY, 0);
            lv_obj_set_style_radius(dot, 2, 0);
            lv_obj_set_style_border_width(dot, 0, 0);
        }
    }
}

// ═══════════════════════════════════════════════════════════════
//  SCREEN 0 — HOME
// ═══════════════════════════════════════════════════════════════
static void ssr_toggle_cb(lv_event_t *e)
{
    // Toggle SSR button appearance
    static bool ssr_on = false;
    ssr_on = !ssr_on;
    lv_obj_t *btn = lv_event_get_target(e);
    if (ssr_on) {
        lv_obj_set_style_bg_color(btn, COLOR_ACCENT_WARM, 0);
        lv_label_set_text(g_lbl_ssr, LV_SYMBOL_POWER "  SSR AV");
        ui_update_ssr_state(true);
    } else {
        lv_obj_set_style_bg_color(btn, COLOR_BG_ELEVATED, 0);
        lv_label_set_text(g_lbl_ssr, LV_SYMBOL_POWER "  SSR PÅ");
        ui_update_ssr_state(false);
    }
}

void ui_home_screen_init(void)
{
    g_screen_home = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(g_screen_home, COLOR_BG_BASE, 0);
    lv_obj_set_style_bg_opa(g_screen_home, LV_OPA_COVER, 0);
    lv_obj_clear_flag(g_screen_home, LV_OBJ_FLAG_SCROLLABLE);

    /* ── Header bar ─────────────────────────────────────────── */
    lv_obj_t *header = lv_obj_create(g_screen_home);
    lv_obj_set_pos(header, 0, 0);
    lv_obj_set_size(header, SCREEN_W, 56);
    lv_obj_set_style_bg_color(header, COLOR_BG_SURFACE, 0);
    lv_obj_set_style_border_width(header, 0, 0);
    lv_obj_set_style_radius(header, 0, 0);
    lv_obj_set_style_pad_all(header, 0, 0);
    lv_obj_clear_flag(header, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *h_title = lv_label_create(header);
    lv_label_set_text(h_title, "Autoklav Control");
    lv_obj_set_style_text_color(h_title, COLOR_TEXT_PRIMARY, 0);
    lv_obj_set_style_text_font(h_title, &lv_font_montserrat_18, 0);
    lv_obj_align(h_title, LV_ALIGN_LEFT_MID, PADDING_LG, 0);

    // Status badge (top right)
    g_lbl_status = lv_label_create(header);
    lv_label_set_text(g_lbl_status, LV_SYMBOL_OK "  Standby");
    lv_obj_set_style_text_color(g_lbl_status, COLOR_ACCENT_GREEN, 0);
    lv_obj_set_style_text_font(g_lbl_status, &lv_font_montserrat_14, 0);
    lv_obj_align(g_lbl_status, LV_ALIGN_RIGHT_MID, -PADDING_LG, 0);

    /* ── Main temperature arc ───────────────────────────────── */
    // Background circle card
    lv_obj_t *arc_card = lv_obj_create(g_screen_home);
    lv_obj_set_pos(arc_card, SCREEN_W/2 - 170, 64);
    lv_obj_set_size(arc_card, 340, 340);
    lv_obj_set_style_bg_color(arc_card, COLOR_BG_SURFACE, 0);
    lv_obj_set_style_radius(arc_card, 170, 0);
    lv_obj_set_style_border_width(arc_card, 0, 0);
    lv_obj_set_style_pad_all(arc_card, 0, 0);
    lv_obj_clear_flag(arc_card, LV_OBJ_FLAG_SCROLLABLE);

    // Outer arc (track)
    lv_obj_t *arc_bg = lv_arc_create(arc_card);
    lv_obj_set_size(arc_bg, 300, 300);
    lv_obj_center(arc_bg);
    lv_arc_set_bg_angles(arc_bg, 135, 405);
    lv_arc_set_value(arc_bg, 0);
    lv_obj_set_style_arc_color(arc_bg, COLOR_BG_ELEVATED, LV_PART_MAIN);
    lv_obj_set_style_arc_width(arc_bg, 18, LV_PART_MAIN);
    lv_obj_set_style_arc_color(arc_bg, COLOR_ACCENT_WARM, LV_PART_INDICATOR);
    lv_obj_set_style_arc_width(arc_bg, 18, LV_PART_INDICATOR);
    lv_obj_set_style_bg_opa(arc_bg, LV_OPA_TRANSP, LV_PART_KNOB);
    lv_obj_set_style_size(arc_bg, 0, 0, LV_PART_KNOB);
    lv_obj_clear_flag(arc_bg, LV_OBJ_FLAG_CLICKABLE);
    g_arc_temp = arc_bg;

    // Temperature value label
    g_lbl_temp_value = lv_label_create(arc_card);
    lv_label_set_text(g_lbl_temp_value, "---");
    lv_obj_set_style_text_font(g_lbl_temp_value, &lv_font_montserrat_48, 0);
    lv_obj_set_style_text_color(g_lbl_temp_value, COLOR_TEXT_PRIMARY, 0);
    lv_obj_align(g_lbl_temp_value, LV_ALIGN_CENTER, 0, -12);

    lv_obj_t *unit_lbl = lv_label_create(arc_card);
    lv_label_set_text(unit_lbl, "°C");
    lv_obj_set_style_text_font(unit_lbl, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(unit_lbl, COLOR_TEXT_SECONDARY, 0);
    lv_obj_align(unit_lbl, LV_ALIGN_CENTER, 0, 28);

    lv_obj_t *temp_title = lv_label_create(arc_card);
    lv_label_set_text(temp_title, "TEMPERATUR");
    lv_obj_set_style_text_font(temp_title, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(temp_title, COLOR_TEXT_DISABLED, 0);
    lv_obj_align_to(temp_title, unit_lbl, LV_ALIGN_OUT_BOTTOM_MID, 0, 6);

    /* ── Bottom row: Pressure + SSR + Setpoint ──────────────── */
    int card_y = 420;
    int card_h = 108;

    // Pressure card
    lv_obj_t *p_card = make_card(g_screen_home, PADDING_MD, card_y, 210, card_h);
    make_card_title(p_card, LV_SYMBOL_WARNING "  TRYCK");
    g_lbl_pressure_value = make_value_label(p_card, "--.-", &lv_font_montserrat_32,
                                             COLOR_PRIMARY);
    lv_obj_align(g_lbl_pressure_value, LV_ALIGN_LEFT_MID, 0, 10);
    lv_obj_t *p_unit = lv_label_create(p_card);
    lv_label_set_text(p_unit, "bar");
    lv_obj_set_style_text_color(p_unit, COLOR_TEXT_SECONDARY, 0);
    lv_obj_set_style_text_font(p_unit, &lv_font_montserrat_14, 0);
    lv_obj_align(p_unit, LV_ALIGN_BOTTOM_RIGHT, 0, 0);

    // Setpoint card
    lv_obj_t *sp_card = make_card(g_screen_home, PADDING_MD + 218, card_y, 210, card_h);
    make_card_title(sp_card, LV_SYMBOL_UP "  MÅLTEMP");
    lv_obj_t *lbl_sp = make_value_label(sp_card, "134", &lv_font_montserrat_32,
                                          COLOR_ACCENT_YELLOW);
    lv_obj_align(lbl_sp, LV_ALIGN_LEFT_MID, 0, 10);
    lv_obj_t *sp_unit = lv_label_create(sp_card);
    lv_label_set_text(sp_unit, "°C");
    lv_obj_set_style_text_color(sp_unit, COLOR_TEXT_SECONDARY, 0);
    lv_obj_set_style_text_font(sp_unit, &lv_font_montserrat_14, 0);
    lv_obj_align(sp_unit, LV_ALIGN_BOTTOM_RIGHT, 0, 0);

    // SSR Toggle card
    lv_obj_t *ssr_card = make_card(g_screen_home, PADDING_MD + 218*2, card_y, 210, card_h);
    make_card_title(ssr_card, "  RELÄ");
    g_btn_ssr = lv_btn_create(ssr_card);
    lv_obj_set_size(g_btn_ssr, 174, 52);
    lv_obj_align(g_btn_ssr, LV_ALIGN_CENTER, 0, 8);
    lv_obj_set_style_bg_color(g_btn_ssr, COLOR_BG_ELEVATED, 0);
    lv_obj_set_style_radius(g_btn_ssr, BTN_RADIUS, 0);
    lv_obj_set_style_border_width(g_btn_ssr, 0, 0);
    lv_obj_add_event_cb(g_btn_ssr, ssr_toggle_cb, LV_EVENT_CLICKED, NULL);
    g_lbl_ssr = lv_label_create(g_btn_ssr);
    lv_label_set_text(g_lbl_ssr, LV_SYMBOL_POWER "  SSR PÅ");
    lv_obj_set_style_text_font(g_lbl_ssr, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(g_lbl_ssr, COLOR_TEXT_PRIMARY, 0);
    lv_obj_center(g_lbl_ssr);

    /* ── Quick action row ───────────────────────────────────── */
    int qa_y = card_y + card_h + PADDING_MD;
    lv_obj_t *qa_card = make_card(g_screen_home, PADDING_MD, qa_y,
                                   SCREEN_W - PADDING_MD*2, 80);
    lv_obj_set_style_bg_color(qa_card, COLOR_BG_SURFACE, 0);

    lv_obj_t *qa_title = lv_label_create(qa_card);
    lv_label_set_text(qa_title, "Snabbstart:");
    lv_obj_set_style_text_color(qa_title, COLOR_TEXT_SECONDARY, 0);
    lv_obj_set_style_text_font(qa_title, &lv_font_montserrat_12, 0);
    lv_obj_align(qa_title, LV_ALIGN_LEFT_MID, 0, 0);

    const char *presets[] = {"134°C / 18min", "121°C / 30min", "Torkning"};
    lv_color_t preset_colors[] = { COLOR_ACCENT_WARM, COLOR_PRIMARY, COLOR_ACCENT_GREEN };
    for (int i = 0; i < 3; i++) {
        lv_obj_t *pb = make_button(qa_card, presets[i], preset_colors[i], 180, 44, NULL);
        lv_obj_align(pb, LV_ALIGN_RIGHT_MID, -(i * 190), 0);
    }

    create_navbar(g_screen_home, 0);
}

// ═══════════════════════════════════════════════════════════════
//  SCREEN 1 — MONITOR
// ═══════════════════════════════════════════════════════════════
void ui_monitor_screen_init(void)
{
    g_screen_monitor = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(g_screen_monitor, COLOR_BG_BASE, 0);
    lv_obj_clear_flag(g_screen_monitor, LV_OBJ_FLAG_SCROLLABLE);

    // Header
    lv_obj_t *hdr = lv_obj_create(g_screen_monitor);
    lv_obj_set_pos(hdr, 0, 0); lv_obj_set_size(hdr, SCREEN_W, 56);
    lv_obj_set_style_bg_color(hdr, COLOR_BG_SURFACE, 0);
    lv_obj_set_style_border_width(hdr, 0, 0);
    lv_obj_set_style_radius(hdr, 0, 0);
    lv_obj_clear_flag(hdr, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_t *hdr_lbl = lv_label_create(hdr);
    lv_label_set_text(hdr_lbl, LV_SYMBOL_CHART "  Realtidsmonitor");
    lv_obj_set_style_text_color(hdr_lbl, COLOR_TEXT_PRIMARY, 0);
    lv_obj_set_style_text_font(hdr_lbl, &lv_font_montserrat_18, 0);
    lv_obj_align(hdr_lbl, LV_ALIGN_LEFT_MID, PADDING_LG, 0);

    /* ── Chart card ─────────────────────────────────────────── */
    lv_obj_t *ch_card = make_card(g_screen_monitor, PADDING_MD, 68,
                                   SCREEN_W - PADDING_MD*2, 320);

    lv_obj_t *ch_title = lv_label_create(ch_card);
    lv_label_set_text(ch_title, "Temperaturhistorik (°C)");
    lv_obj_set_style_text_color(ch_title, COLOR_TEXT_SECONDARY, 0);
    lv_obj_set_style_text_font(ch_title, &lv_font_montserrat_14, 0);
    lv_obj_align(ch_title, LV_ALIGN_TOP_LEFT, 0, 0);

    g_chart_temp = lv_chart_create(ch_card);
    lv_obj_set_size(g_chart_temp, lv_pct(100), 265);
    lv_obj_align(g_chart_temp, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_chart_set_type(g_chart_temp, LV_CHART_TYPE_LINE);
    lv_chart_set_point_count(g_chart_temp, 60);
    lv_chart_set_range(g_chart_temp, LV_CHART_AXIS_PRIMARY_Y, 0, 150);

    lv_obj_set_style_bg_color(g_chart_temp, COLOR_BG_SURFACE, 0);
    lv_obj_set_style_bg_opa(g_chart_temp, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(g_chart_temp, 1, 0);
    lv_obj_set_style_border_color(g_chart_temp, COLOR_DIVIDER, 0);
    lv_obj_set_style_line_color(g_chart_temp, COLOR_DIVIDER, LV_PART_MAIN);
    lv_chart_set_div_line_count(g_chart_temp, 5, 10);

    g_ser_temp = lv_chart_add_series(g_chart_temp, COLOR_ACCENT_WARM,
                                      LV_CHART_AXIS_PRIMARY_Y);
    lv_obj_set_style_line_width(g_chart_temp, 3, LV_PART_ITEMS);
    lv_obj_set_style_size(g_chart_temp, 0, 0, LV_PART_INDICATOR);

    // Add pressure series (secondary)
    lv_chart_series_t *ser_p = lv_chart_add_series(g_chart_temp, COLOR_PRIMARY,
                                                     LV_CHART_AXIS_PRIMARY_Y);
    (void)ser_p;

    /* ── Stats row ──────────────────────────────────────────── */
    int sy = 68 + 320 + PADDING_MD;
    struct { const char *lbl; const char *val; lv_color_t c; } stats[] = {
        { "Min Temp", "22.1 °C",  COLOR_PRIMARY },
        { "Max Temp", "136.2 °C", COLOR_ACCENT_WARM },
        { "Cycle Time","0:00:00", COLOR_ACCENT_GREEN },
        { "Larm",     "0",        COLOR_ACCENT_GREEN },
    };
    int sw = (SCREEN_W - PADDING_MD*5) / 4;
    for (int i = 0; i < 4; i++) {
        lv_obj_t *sc = make_card(g_screen_monitor,
                                   PADDING_MD + i*(sw+PADDING_MD), sy, sw, 72);
        lv_obj_t *sl = lv_label_create(sc);
        lv_label_set_text(sl, stats[i].lbl);
        lv_obj_set_style_text_color(sl, COLOR_TEXT_SECONDARY, 0);
        lv_obj_set_style_text_font(sl, &lv_font_montserrat_10, 0);
        lv_obj_align(sl, LV_ALIGN_TOP_LEFT, 0, 0);
        lv_obj_t *sv = lv_label_create(sc);
        lv_label_set_text(sv, stats[i].val);
        lv_obj_set_style_text_color(sv, stats[i].c, 0);
        lv_obj_set_style_text_font(sv, &lv_font_montserrat_18, 0);
        lv_obj_align(sv, LV_ALIGN_BOTTOM_LEFT, 0, 0);
    }

    /* ── Log list ───────────────────────────────────────────── */
    int ly = sy + 72 + PADDING_MD;
    lv_obj_t *log_card = make_card(g_screen_monitor, PADDING_MD, ly,
                                    SCREEN_W - PADDING_MD*2,
                                    CONTENT_H - ly - PADDING_MD);
    lv_obj_set_style_pad_all(log_card, PADDING_SM, 0);

    lv_obj_t *log_title = lv_label_create(log_card);
    lv_label_set_text(log_title, LV_SYMBOL_LIST "  Händelselogg");
    lv_obj_set_style_text_color(log_title, COLOR_TEXT_SECONDARY, 0);
    lv_obj_set_style_text_font(log_title, &lv_font_montserrat_14, 0);
    lv_obj_align(log_title, LV_ALIGN_TOP_LEFT, 0, 0);

    g_log_list = lv_list_create(log_card);
    lv_obj_set_size(g_log_list, lv_pct(100),
                    lv_obj_get_height(log_card) - 28);
    lv_obj_align(g_log_list, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_set_style_bg_color(g_log_list, COLOR_BG_SURFACE, 0);
    lv_obj_set_style_border_width(g_log_list, 0, 0);
    lv_obj_set_style_text_color(g_log_list, COLOR_TEXT_PRIMARY, 0);

    // Seed with a couple initial log entries
    ui_add_log_entry(LV_SYMBOL_OK "  System startat");
    ui_add_log_entry(LV_SYMBOL_WARNING "  Väntar på uppvärmning...");

    create_navbar(g_screen_monitor, 1);
}

// ═══════════════════════════════════════════════════════════════
//  SCREEN 2 — PROGRAMS
// ═══════════════════════════════════════════════════════════════
typedef struct {
    const char *name;
    const char *temp;
    const char *time;
    const char *pressure;
    const char *desc;
    lv_color_t color;
} CycleProgram;

static const CycleProgram PROGRAMS[] = {
    { "Steril 134°C",  "134°C",  "18 min",  "2.1 bar", "Standard-autoklavering\nför metallinstrument", COLOR_ACCENT_WARM },
    { "Steril 121°C",  "121°C",  "30 min",  "1.1 bar", "Långsam cykel för\nkänsligt material",        COLOR_PRIMARY },
    { "Flash-steril",  "134°C",  "4 min",   "2.1 bar", "Snabb cykel för\noförpackade instrument",     COLOR_ACCENT_YELLOW },
    { "Torkcykel",     "115°C",  "20 min",  "0.7 bar", "Torkning utan\ntryckuppbyggnad",              COLOR_ACCENT_GREEN },
};

static void program_start_cb(lv_event_t *e)
{
    // In real code: send event to control task
    lv_obj_t *btn = lv_event_get_target(e);
    lv_obj_t *lbl = lv_obj_get_child(btn, 0);
    lv_label_set_text(lbl, LV_SYMBOL_PLAY "  Kör...");
    lv_obj_set_style_bg_color(btn, COLOR_ACCENT_GREEN, 0);
}

void ui_programs_screen_init(void)
{
    g_screen_programs = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(g_screen_programs, COLOR_BG_BASE, 0);
    lv_obj_clear_flag(g_screen_programs, LV_OBJ_FLAG_SCROLLABLE);

    // Header
    lv_obj_t *hdr = lv_obj_create(g_screen_programs);
    lv_obj_set_pos(hdr, 0, 0); lv_obj_set_size(hdr, SCREEN_W, 56);
    lv_obj_set_style_bg_color(hdr, COLOR_BG_SURFACE, 0);
    lv_obj_set_style_border_width(hdr, 0, 0);
    lv_obj_set_style_radius(hdr, 0, 0);
    lv_obj_clear_flag(hdr, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_t *hdr_l = lv_label_create(hdr);
    lv_label_set_text(hdr_l, LV_SYMBOL_LIST "  Steriliseringsprogram");
    lv_obj_set_style_text_color(hdr_l, COLOR_TEXT_PRIMARY, 0);
    lv_obj_set_style_text_font(hdr_l, &lv_font_montserrat_18, 0);
    lv_obj_align(hdr_l, LV_ALIGN_LEFT_MID, PADDING_LG, 0);

    // Program cards
    int n = sizeof(PROGRAMS) / sizeof(PROGRAMS[0]);
    int card_h = (CONTENT_H - 56 - PADDING_MD*(n+1)) / n;
    for (int i = 0; i < n; i++) {
        int y = 56 + PADDING_MD + i * (card_h + PADDING_MD);
        lv_obj_t *pc = make_card(g_screen_programs,
                                  PADDING_MD, y,
                                  SCREEN_W - PADDING_MD*2, card_h);

        // Coloured left accent bar
        lv_obj_t *bar = lv_obj_create(pc);
        lv_obj_set_pos(bar, -PADDING_MD, -PADDING_MD);
        lv_obj_set_size(bar, 4, card_h);
        lv_obj_set_style_bg_color(bar, PROGRAMS[i].color, 0);
        lv_obj_set_style_radius(bar, 0, 0);
        lv_obj_set_style_border_width(bar, 0, 0);

        // Program name
        lv_obj_t *pn = lv_label_create(pc);
        lv_label_set_text(pn, PROGRAMS[i].name);
        lv_obj_set_style_text_color(pn, COLOR_TEXT_PRIMARY, 0);
        lv_obj_set_style_text_font(pn, &lv_font_montserrat_18, 0);
        lv_obj_align(pn, LV_ALIGN_TOP_LEFT, 12, 0);

        // Description
        lv_obj_t *pd = lv_label_create(pc);
        lv_label_set_text(pd, PROGRAMS[i].desc);
        lv_obj_set_style_text_color(pd, COLOR_TEXT_SECONDARY, 0);
        lv_obj_set_style_text_font(pd, &lv_font_montserrat_12, 0);
        lv_obj_align(pd, LV_ALIGN_TOP_LEFT, 12, 26);

        // Specs (temp / time / pressure)
        char spec[64];
        snprintf(spec, sizeof(spec), "%s  |  %s  |  %s",
                 PROGRAMS[i].temp, PROGRAMS[i].time, PROGRAMS[i].pressure);
        lv_obj_t *ps = lv_label_create(pc);
        lv_label_set_text(ps, spec);
        lv_obj_set_style_text_color(ps, PROGRAMS[i].color, 0);
        lv_obj_set_style_text_font(ps, &lv_font_montserrat_12, 0);
        lv_obj_align(ps, LV_ALIGN_BOTTOM_LEFT, 12, 0);

        // Start button
        lv_obj_t *sb = make_button(pc, LV_SYMBOL_PLAY "  Starta",
                                    PROGRAMS[i].color, 140, 40,
                                    program_start_cb);
        lv_obj_align(sb, LV_ALIGN_RIGHT_MID, 0, 0);
    }

    create_navbar(g_screen_programs, 2);
}

// ═══════════════════════════════════════════════════════════════
//  SCREEN 3 — SETTINGS
// ═══════════════════════════════════════════════════════════════
static void slider_pid_cb(lv_event_t *e)
{
    lv_obj_t *sl = lv_event_get_target(e);
    lv_obj_t *lbl = (lv_obj_t *)lv_event_get_user_data(e);
    char buf[16];
    snprintf(buf, sizeof(buf), "%.1f", (float)lv_slider_get_value(sl) / 10.0f);
    lv_label_set_text(lbl, buf);
}

static lv_obj_t *make_pid_row(lv_obj_t *parent, const char *name,
                               float init_val, int y,
                               lv_obj_t **out_slider, lv_obj_t **out_lbl)
{
    lv_obj_t *row = lv_obj_create(parent);
    lv_obj_set_pos(row, 0, y);
    lv_obj_set_size(row, lv_pct(100), 56);
    lv_obj_set_style_bg_opa(row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(row, 0, 0);
    lv_obj_set_style_pad_all(row, 0, 0);
    lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *name_lbl = lv_label_create(row);
    lv_label_set_text(name_lbl, name);
    lv_obj_set_style_text_color(name_lbl, COLOR_TEXT_PRIMARY, 0);
    lv_obj_set_style_text_font(name_lbl, &lv_font_montserrat_16, 0);
    lv_obj_align(name_lbl, LV_ALIGN_LEFT_MID, 0, 0);

    lv_obj_t *val_lbl = lv_label_create(row);
    char buf[16];
    snprintf(buf, sizeof(buf), "%.1f", init_val);
    lv_label_set_text(val_lbl, buf);
    lv_obj_set_style_text_color(val_lbl, COLOR_PRIMARY, 0);
    lv_obj_set_style_text_font(val_lbl, &lv_font_montserrat_16, 0);
    lv_obj_align(val_lbl, LV_ALIGN_RIGHT_MID, 0, 0);

    lv_obj_t *sl = lv_slider_create(row);
    lv_obj_set_size(sl, lv_pct(60), 8);
    lv_obj_align(sl, LV_ALIGN_CENTER, -20, 0);
    lv_slider_set_range(sl, 0, 1000);
    lv_slider_set_value(sl, (int)(init_val * 10), LV_ANIM_OFF);
    lv_obj_set_style_bg_color(sl, COLOR_BG_ELEVATED, LV_PART_MAIN);
    lv_obj_set_style_bg_color(sl, COLOR_PRIMARY, LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(sl, COLOR_TEXT_PRIMARY, LV_PART_KNOB);
    lv_obj_set_style_radius(sl, 4, LV_PART_MAIN);
    lv_obj_add_event_cb(sl, slider_pid_cb, LV_EVENT_VALUE_CHANGED, val_lbl);

    if (out_slider) *out_slider = sl;
    if (out_lbl)    *out_lbl    = val_lbl;
    return row;
}

void ui_settings_screen_init(void)
{
    g_screen_settings = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(g_screen_settings, COLOR_BG_BASE, 0);
    lv_obj_clear_flag(g_screen_settings, LV_OBJ_FLAG_SCROLLABLE);

    // Header
    lv_obj_t *hdr = lv_obj_create(g_screen_settings);
    lv_obj_set_pos(hdr, 0, 0); lv_obj_set_size(hdr, SCREEN_W, 56);
    lv_obj_set_style_bg_color(hdr, COLOR_BG_SURFACE, 0);
    lv_obj_set_style_border_width(hdr, 0, 0);
    lv_obj_set_style_radius(hdr, 0, 0);
    lv_obj_clear_flag(hdr, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_t *hdr_l = lv_label_create(hdr);
    lv_label_set_text(hdr_l, LV_SYMBOL_SETTINGS "  Inställningar");
    lv_obj_set_style_text_color(hdr_l, COLOR_TEXT_PRIMARY, 0);
    lv_obj_set_style_text_font(hdr_l, &lv_font_montserrat_18, 0);
    lv_obj_align(hdr_l, LV_ALIGN_LEFT_MID, PADDING_LG, 0);

    /* ── Tabview ─────────────────────────────────────────────── */
    lv_obj_t *tv = lv_tabview_create(g_screen_settings);
    lv_obj_set_pos(tv, 0, 56);
    lv_obj_set_size(tv, SCREEN_W, CONTENT_H - 56);
    lv_tabview_set_tab_bar_position(tv, LV_DIR_TOP);
    lv_tabview_set_tab_bar_size(tv, 44);

    lv_obj_set_style_bg_color(tv, COLOR_BG_BASE, 0);
    lv_obj_set_style_bg_color(lv_tabview_get_tab_bar(tv), COLOR_BG_SURFACE, 0);
    lv_obj_set_style_border_width(lv_tabview_get_tab_bar(tv), 0, 0);

    // Tab button styles
    lv_obj_set_style_text_color(lv_tabview_get_tab_bar(tv), COLOR_TEXT_SECONDARY, 0);
    lv_obj_set_style_text_color(lv_tabview_get_tab_bar(tv), COLOR_PRIMARY,
                                 LV_STATE_CHECKED);

    lv_obj_t *tab_pid  = lv_tabview_add_tab(tv, "PID");
    lv_obj_t *tab_net  = lv_tabview_add_tab(tv, "Nätverk");
    lv_obj_t *tab_sys  = lv_tabview_add_tab(tv, "System");

    // ─── PID TAB ─────────────────────────────────────────────
    lv_obj_set_style_bg_color(tab_pid, COLOR_BG_BASE, 0);
    lv_obj_set_style_pad_all(tab_pid, PADDING_MD, 0);

    lv_obj_t *pid_card = make_card(tab_pid, 0, 0,
                                    lv_pct(100), 220);
    lv_obj_set_style_pad_all(pid_card, PADDING_LG, 0);

    lv_obj_t *pid_title = lv_label_create(pid_card);
    lv_label_set_text(pid_title, "PID-parametrar (Temperaturreglering)");
    lv_obj_set_style_text_color(pid_title, COLOR_TEXT_SECONDARY, 0);
    lv_obj_set_style_text_font(pid_title, &lv_font_montserrat_14, 0);
    lv_obj_align(pid_title, LV_ALIGN_TOP_LEFT, 0, 0);

    make_pid_row(pid_card, "Kp  (Proportional)", 2.5f, 32,
                 &g_slider_kp, &g_lbl_kp_val);
    make_pid_row(pid_card, "Ki  (Integral)",      0.8f, 94,
                 &g_slider_ki, &g_lbl_ki_val);
    make_pid_row(pid_card, "Kd  (Derivata)",      0.3f, 156,
                 &g_slider_kd, &g_lbl_kd_val);

    // Setpoint roller
    lv_obj_t *sp_card = make_card(tab_pid, 0, 228, lv_pct(100), 100);
    lv_obj_t *sp_lbl  = lv_label_create(sp_card);
    lv_label_set_text(sp_lbl, "Måltemperatur (°C)");
    lv_obj_set_style_text_color(sp_lbl, COLOR_TEXT_SECONDARY, 0);
    lv_obj_set_style_text_font(sp_lbl, &lv_font_montserrat_14, 0);
    lv_obj_align(sp_lbl, LV_ALIGN_TOP_LEFT, 0, 0);

    lv_obj_t *sp_roller = lv_roller_create(sp_card);
    lv_roller_set_options(sp_roller,
        "100\n105\n110\n115\n120\n121\n125\n130\n134\n135\n140",
        LV_ROLLER_MODE_NORMAL);
    lv_roller_set_selected(sp_roller, 8, LV_ANIM_OFF); // 134
    lv_obj_set_size(sp_roller, 120, 64);
    lv_obj_align(sp_roller, LV_ALIGN_RIGHT_MID, 0, 8);
    lv_obj_set_style_bg_color(sp_roller, COLOR_BG_ELEVATED, 0);
    lv_obj_set_style_text_color(sp_roller, COLOR_TEXT_PRIMARY, 0);
    lv_obj_set_style_text_color(sp_roller, COLOR_PRIMARY, LV_PART_SELECTED);
    lv_obj_set_style_bg_color(sp_roller, COLOR_BG_ELEVATED, LV_PART_SELECTED);
    lv_obj_set_style_border_width(sp_roller, 0, 0);

    // Save button
    lv_obj_t *save_btn = make_button(tab_pid, LV_SYMBOL_SAVE "  Spara PID",
                                      COLOR_PRIMARY, 200, 44, NULL);
    lv_obj_align(save_btn, LV_ALIGN_BOTTOM_MID, 0, -PADDING_MD);

    // ─── NETWORK TAB ─────────────────────────────────────────
    lv_obj_set_style_bg_color(tab_net, COLOR_BG_BASE, 0);
    lv_obj_set_style_pad_all(tab_net, PADDING_MD, 0);

    lv_obj_t *net_card = make_card(tab_net, 0, 0, lv_pct(100), lv_pct(90));
    lv_obj_set_style_pad_all(net_card, PADDING_LG, 0);

    struct { const char *k; const char *v; } net_rows[] = {
        { "Ethernet",   "Ansluten (1 Gbps)" },
        { "IP-adress",  "192.168.1.42" },
        { "Gateway",    "192.168.1.1" },
        { "DNS",        "8.8.8.8" },
        { "Hostname",   "autoklav-01" },
        { "MQTT Broker","mqtt://192.168.1.10:1883" },
        { "MQTT Status","Ansluten" },
    };
    int nr = sizeof(net_rows)/sizeof(net_rows[0]);
    for (int i = 0; i < nr; i++) {
        lv_obj_t *row = lv_obj_create(net_card);
        lv_obj_set_size(row, lv_pct(100), 40);
        lv_obj_set_pos(row, 0, 4 + i * 44);
        lv_obj_set_style_bg_opa(row, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_width(row, 0, 0);
        lv_obj_set_style_border_side(row, LV_BORDER_SIDE_BOTTOM, 0);
        lv_obj_set_style_border_color(row, COLOR_DIVIDER, 0);
        if (i < nr-1)
            lv_obj_set_style_border_width(row, 1, 0);
        lv_obj_set_style_pad_all(row, 0, 0);
        lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);

        lv_obj_t *kl = lv_label_create(row);
        lv_label_set_text(kl, net_rows[i].k);
        lv_obj_set_style_text_color(kl, COLOR_TEXT_SECONDARY, 0);
        lv_obj_set_style_text_font(kl, &lv_font_montserrat_14, 0);
        lv_obj_align(kl, LV_ALIGN_LEFT_MID, 0, 0);

        lv_obj_t *vl = lv_label_create(row);
        lv_label_set_text(vl, net_rows[i].v);
        lv_obj_set_style_text_color(vl, COLOR_TEXT_PRIMARY, 0);
        lv_obj_set_style_text_font(vl, &lv_font_montserrat_14, 0);
        lv_obj_align(vl, LV_ALIGN_RIGHT_MID, 0, 0);
    }

    // ─── SYSTEM TAB ──────────────────────────────────────────
    lv_obj_set_style_bg_color(tab_sys, COLOR_BG_BASE, 0);
    lv_obj_set_style_pad_all(tab_sys, PADDING_MD, 0);

    lv_obj_t *sys_card = make_card(tab_sys, 0, 0, lv_pct(100), 200);
    lv_obj_set_style_pad_all(sys_card, PADDING_LG, 0);

    struct { const char *k; const char *v; lv_color_t c; } sys_rows[] = {
        { "Hårdvara",   "ESP32-P4-86-Panel-ETH-2RO",   COLOR_TEXT_PRIMARY },
        { "CPU",        "RISC-V Dual-core @ 400 MHz",   COLOR_TEXT_PRIMARY },
        { "PSRAM",      "32 MB",                         COLOR_ACCENT_GREEN },
        { "Flash",      "32 MB",                         COLOR_ACCENT_GREEN },
        { "Display",    "720×720 IPS MIPI-DSI",          COLOR_PRIMARY },
        { "FW Version", "v1.0.0-dev",                    COLOR_TEXT_SECONDARY },
    };
    for (int i = 0; i < 6; i++) {
        lv_obj_t *row = lv_obj_create(sys_card);
        lv_obj_set_size(row, lv_pct(100), 28);
        lv_obj_set_pos(row, 0, i * 30);
        lv_obj_set_style_bg_opa(row, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_width(row, 0, 0);
        lv_obj_set_style_pad_all(row, 0, 0);
        lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_t *kl = lv_label_create(row);
        lv_label_set_text(kl, sys_rows[i].k);
        lv_obj_set_style_text_color(kl, COLOR_TEXT_SECONDARY, 0);
        lv_obj_set_style_text_font(kl, &lv_font_montserrat_13, 0);
        lv_obj_align(kl, LV_ALIGN_LEFT_MID, 0, 0);
        lv_obj_t *vl = lv_label_create(row);
        lv_label_set_text(vl, sys_rows[i].v);
        lv_obj_set_style_text_color(vl, sys_rows[i].c, 0);
        lv_obj_set_style_text_font(vl, &lv_font_montserrat_13, 0);
        lv_obj_align(vl, LV_ALIGN_RIGHT_MID, 0, 0);
    }

    // Action buttons
    lv_obj_t *reboot_btn = make_button(tab_sys, LV_SYMBOL_REFRESH "  Starta om",
                                        COLOR_ACCENT_YELLOW, 200, 44, NULL);
    lv_obj_align(reboot_btn, LV_ALIGN_BOTTOM_LEFT, 0, -PADDING_MD);

    lv_obj_t *reset_btn  = make_button(tab_sys, LV_SYMBOL_CLOSE "  Fabriksåterst.",
                                        COLOR_ACCENT_RED, 200, 44, NULL);
    lv_obj_align(reset_btn, LV_ALIGN_BOTTOM_RIGHT, 0, -PADDING_MD);

    create_navbar(g_screen_settings, 3);
}

// ═══════════════════════════════════════════════════════════════
//  NAVIGATION
// ═══════════════════════════════════════════════════════════════
void ui_navigate_to(int idx)
{
    lv_obj_t *screens[] = {
        g_screen_home,
        g_screen_monitor,
        g_screen_programs,
        g_screen_settings
    };
    if (idx < 0 || idx > 3) return;
    g_active_screen = idx;
    lv_scr_load_anim(screens[idx], LV_SCR_LOAD_ANIM_FADE_ON, 200, 0, false);
}

// ═══════════════════════════════════════════════════════════════
//  INIT
// ═══════════════════════════════════════════════════════════════
void ui_init(void)
{
    // Enable montserrat fonts in lv_conf.h:
    //   LV_FONT_MONTSERRAT_10, 12, 13, 14, 16, 18, 20, 48 = 1

    ui_home_screen_init();
    ui_monitor_screen_init();
    ui_programs_screen_init();
    ui_settings_screen_init();

    lv_scr_load(g_screen_home);
}

// ═══════════════════════════════════════════════════════════════
//  LIVE DATA UPDATE API
// ═══════════════════════════════════════════════════════════════
void ui_update_temperature(float temp_c)
{
    if (!g_arc_temp || !g_lbl_temp_value) return;

    // Arc range 0–150°C mapped to 0–100 arc value
    int arc_val = (int)(temp_c / 150.0f * 100.0f);
    if (arc_val < 0)   arc_val = 0;
    if (arc_val > 100) arc_val = 100;
    lv_arc_set_value(g_arc_temp, arc_val);

    // Colour: blue→cyan→orange→red
    lv_color_t col;
    if      (temp_c < 80)  col = COLOR_PRIMARY;
    else if (temp_c < 120) col = COLOR_ACCENT_YELLOW;
    else if (temp_c < 140) col = COLOR_ACCENT_WARM;
    else                   col = COLOR_ACCENT_RED;
    lv_obj_set_style_arc_color(g_arc_temp, col, LV_PART_INDICATOR);

    char buf[16];
    snprintf(buf, sizeof(buf), "%.1f", temp_c);
    lv_label_set_text(g_lbl_temp_value, buf);

    // Also push to chart
    if (g_chart_temp && g_ser_temp) {
        lv_chart_set_next_value(g_chart_temp, g_ser_temp, (lv_coord_t)temp_c);
    }
}

void ui_update_pressure(float bar)
{
    if (!g_lbl_pressure_value) return;
    char buf[16];
    snprintf(buf, sizeof(buf), "%.2f", bar);
    lv_label_set_text(g_lbl_pressure_value, buf);
}

void ui_update_ssr_state(bool active)
{
    // Called from ssr_toggle_cb or from control task
    (void)active; // GPIO handling done in control task
}

void ui_update_status(const char *status_text)
{
    if (g_lbl_status)
        lv_label_set_text(g_lbl_status, status_text);
}

void ui_add_log_entry(const char *msg)
{
    if (!g_log_list) return;
    lv_obj_t *btn = lv_list_add_button(g_log_list, NULL, msg);
    lv_obj_set_style_bg_color(btn, COLOR_BG_SURFACE, 0);
    lv_obj_set_style_text_color(btn, COLOR_TEXT_PRIMARY, 0);
    lv_obj_set_style_text_font(btn, &lv_font_montserrat_12, 0);
    lv_obj_set_style_border_width(btn, 0, 0);
    // Auto-scroll to bottom
    lv_obj_scroll_to_y(g_log_list, LV_COORD_MAX, LV_ANIM_ON);
}
