#pragma once

#include <Arduino.h>
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
#define COLOR_TEXT_SECONDARY   lv_color_hex(0xB0BEC5) // サブテキスト
#define COLOR_TEXT_MUTED       lv_color_hex(0x78909C) // 補足テキスト
#define COLOR_STATUS_OK        lv_color_hex(0x00E676) // ステータス 正常 (Green)
#define COLOR_STATUS_WARN      lv_color_hex(0xFFD600) // ステータス 接続中 (Yellow)
#define COLOR_STATUS_ERR       lv_color_hex(0xFF1744) // ステータス エラー (Red)

class UITheme {
public:
    static void initStyles();
    static bool loadJapaneseFontFromSD(const char* ttfPath = "/NotoSansJP-Regular.ttf");

    static lv_style_t style_panel;
    static lv_style_t style_btn_primary;
    static lv_style_t style_tab_btn;

    // 日本語フォントポインタ
    static const lv_font_t* font_small_14;
    static const lv_font_t* font_body_18;
    static const lv_font_t* font_title_24;
    static const lv_font_t* font_date_28;   // 拡大日付・曜日用 (28px)
    static const lv_font_t* font_clock_64;  // 特大デジタル時計用 (64px)

private:
    static uint8_t* _fontDataBuffer;
    static size_t _fontDataSize;
};
