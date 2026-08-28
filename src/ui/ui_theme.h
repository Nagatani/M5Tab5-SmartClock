#pragma once

#include <lvgl.h>

// ==========================================
// Modern Dark Color Palette
// ==========================================
#define COLOR_BG_MAIN          lv_color_hex(0x121418) // メイン背景 (Dark Charcoal)
#define COLOR_PANEL_BG         lv_color_hex(0x1C2028) // カード/パネル背景
#define COLOR_PANEL_BORDER     lv_color_hex(0x2D3442) // 境界線
#define COLOR_ACCENT_CYAN      lv_color_hex(0x00E5FF) // 時計・ハイライト用シアン
#define COLOR_ACCENT_BLUE      lv_color_hex(0x2979FF) // ボタン用ブルー
#define COLOR_ACCENT_ORANGE    lv_color_hex(0xFF9100) // 警告・カテゴリ用オレンジ
#define COLOR_TEXT_PRIMARY     lv_color_hex(0xFFFFFF) // 主要テキスト
#define COLOR_TEXT_SECONDARY   lv_color_hex(0x9E9E9E) // サブテキスト
#define COLOR_TEXT_MUTED       lv_color_hex(0x616161) // 補足テキスト
#define COLOR_STATUS_OK        lv_color_hex(0x00E676) // ステータス 正常 (Green)
#define COLOR_STATUS_WARN      lv_color_hex(0xFFD600) // ステータス 接続中 (Yellow)
#define COLOR_STATUS_ERR       lv_color_hex(0xFF1744) // ステータス エラー (Red)

class UITheme {
public:
    static void initStyles() {
        // パネル基本スタイル
        lv_style_init(&style_panel);
        lv_style_set_bg_color(&style_panel, COLOR_PANEL_BG);
        lv_style_set_bg_opa(&style_panel, LV_OPA_COVER);
        lv_style_set_border_color(&style_panel, COLOR_PANEL_BORDER);
        lv_style_set_border_width(&style_panel, 1);
        lv_style_set_radius(&style_panel, 16);
        lv_style_set_pad_all(&style_panel, 16);

        // ボタンスタイル
        lv_style_init(&style_btn_primary);
        lv_style_set_bg_color(&style_btn_primary, COLOR_ACCENT_BLUE);
        lv_style_set_radius(&style_btn_primary, 12);
        lv_style_set_shadow_width(&style_btn_primary, 8);
        lv_style_set_shadow_color(&style_btn_primary, lv_color_hex(0x000000));
        lv_style_set_shadow_opa(&style_btn_primary, LV_OPA_30);

        // タブボタンスタイル
        lv_style_init(&style_tab_btn);
        lv_style_set_bg_color(&style_tab_btn, lv_color_hex(0x252A36));
        lv_style_set_text_color(&style_tab_btn, COLOR_TEXT_PRIMARY);
        lv_style_set_radius(&style_tab_btn, 8);
    }

    static lv_style_t style_panel;
    static lv_style_t style_btn_primary;
    static lv_style_t style_tab_btn;
};
