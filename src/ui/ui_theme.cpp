#include "ui_theme.h"
#include <SD_MMC.h>
#include <esp_heap_caps.h>

lv_style_t UITheme::style_panel;
lv_style_t UITheme::style_btn_primary;
lv_style_t UITheme::style_tab_btn;

// 日本語フォント (初期フォールバックはMontserrat)
const lv_font_t* UITheme::font_small_14 = &lv_font_montserrat_14;
const lv_font_t* UITheme::font_body_18  = &lv_font_montserrat_18;
const lv_font_t* UITheme::font_title_24 = &lv_font_montserrat_24;

// デジタル時計・日付用 (内蔵 Montserrat フォント: 高速・100% 安定)
const lv_font_t* UITheme::font_clock_large = &lv_font_montserrat_48;
const lv_font_t* UITheme::font_date_large  = &lv_font_montserrat_24;

uint8_t* UITheme::_fontDataBuffer = nullptr;
size_t UITheme::_fontDataSize = 0;

void UITheme::initStyles() {
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

bool UITheme::loadJapaneseFontFromSD(const char* ttfPath) {
    Serial.printf("[Font] Initializing SD_MMC for Japanese font: %s ...\n", ttfPath);

    // M5Stack Tab5 の SD カードは SD_MMC (SDIO) で接続されています
    bool mounted = SD_MMC.begin("/sdcard", true); // 1-bit モード
    if (!mounted) {
        mounted = SD_MMC.begin("/sdcard", false); // 4-bit モード試行
    }

    if (!mounted) {
        Serial.println("[Font] SD_MMC Card not mounted. Please check if MicroSD card is inserted.");
        return false;
    }

    Serial.println("[Font] SD_MMC Card mounted successfully.");

    if (!SD_MMC.exists(ttfPath)) {
        Serial.printf("[Font] Font file not found on SD card: %s\n", ttfPath);
        return false;
    }

    File fontFile = SD_MMC.open(ttfPath, FILE_READ);
    if (!fontFile) {
        Serial.println("[Font] Failed to open font file.");
        return false;
    }

    _fontDataSize = fontFile.size();
    Serial.printf("[Font] Loading TTF font (%u bytes) into PSRAM...\n", (unsigned int)_fontDataSize);

    // 大容量 PSRAM (32MB) にフォントバイナリを確保
    _fontDataBuffer = (uint8_t*)heap_caps_malloc(_fontDataSize, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (!_fontDataBuffer) {
        _fontDataBuffer = (uint8_t*)malloc(_fontDataSize);
    }

    if (!_fontDataBuffer) {
        Serial.println("[Font] Memory allocation failed for font buffer.");
        fontFile.close();
        return false;
    }

    size_t bytesRead = fontFile.read(_fontDataBuffer, _fontDataSize);
    fontFile.close();

    if (bytesRead != _fontDataSize) {
        Serial.println("[Font] Incomplete font file read.");
        return false;
    }

    // Tiny_TTF による安全で軽量な 3 サイズのみ生成 (14px, 18px, 24px)
    static lv_font_t* font14 = lv_tiny_ttf_create_data(_fontDataBuffer, _fontDataSize, 14);
    static lv_font_t* font18 = lv_tiny_ttf_create_data(_fontDataBuffer, _fontDataSize, 18);
    static lv_font_t* font24 = lv_tiny_ttf_create_data(_fontDataBuffer, _fontDataSize, 24);

    if (font14 && font18 && font24) {
        font_small_14 = font14;
        font_body_18  = font18;
        font_title_24 = font24;
        Serial.println("[Font] Japanese TTF font loaded successfully (14px, 18px, 24px)!");
        return true;
    }

    Serial.println("[Font] Tiny_TTF failed to generate font objects.");
    return false;
}
