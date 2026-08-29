#include "ui_clock.h"
#include "ui_theme.h"
#include <esp_heap_caps.h>

#define CANVAS_WIDTH  250
#define CANVAS_HEIGHT 60

static void btn_speak_event_cb(lv_event_t* e) {
    if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
        UIClock::getInstance().handleSpeakTimeBtn();
    }
}

void UIClock::init(lv_obj_t* parent) {
    // メインパネル作成 (左側 55% -> 704 x 720)
    _panel = lv_obj_create(parent);
    lv_obj_set_pos(_panel, 0, 0);
    lv_obj_set_size(_panel, CLOCK_PANEL_WIDTH, SCREEN_HEIGHT);
    lv_obj_add_style(_panel, &UITheme::style_panel, 0);
    lv_obj_clear_flag(_panel, LV_OBJ_FLAG_SCROLLABLE);

    // ==========================================
    // 1. ステータスバー (上部 y: 0〜28)
    // ==========================================
    lv_obj_t* statusBar = lv_obj_create(_panel);
    lv_obj_set_size(statusBar, lv_pct(100), 28);
    lv_obj_set_pos(statusBar, 0, 0);
    lv_obj_set_style_bg_opa(statusBar, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(statusBar, 0, 0);
    lv_obj_set_style_pad_all(statusBar, 0, 0);
    lv_obj_set_flex_flow(statusBar, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(statusBar, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    _lblWifiStatus = lv_label_create(statusBar);
    lv_obj_set_style_text_font(_lblWifiStatus, &lv_font_montserrat_14, 0);
    lv_label_set_text(_lblWifiStatus, LV_SYMBOL_WIFI " Connecting...");
    lv_obj_set_style_text_color(_lblWifiStatus, COLOR_TEXT_SECONDARY, 0);

    _lblRtcStatus = lv_label_create(statusBar);
    lv_obj_set_style_text_font(_lblRtcStatus, &lv_font_montserrat_14, 0);
    lv_label_set_text(_lblRtcStatus, "RTC: OK");
    lv_obj_set_style_text_color(_lblRtcStatus, COLOR_STATUS_OK, 0);

    _lblLlmStatus = lv_label_create(statusBar);
    lv_obj_set_style_text_font(_lblLlmStatus, &lv_font_montserrat_14, 0);
    lv_label_set_text(_lblLlmStatus, "LLM: Standby");
    lv_obj_set_style_text_color(_lblLlmStatus, COLOR_TEXT_SECONDARY, 0);

    _lblBattery = lv_label_create(statusBar);
    lv_obj_set_style_text_font(_lblBattery, &lv_font_montserrat_14, 0);
    lv_label_set_text(_lblBattery, LV_SYMBOL_BATTERY_FULL " 100%");
    lv_obj_set_style_text_color(_lblBattery, COLOR_TEXT_SECONDARY, 0);

    // ==========================================
    // 2. 特大日付・曜日表示 (48px Montserrat フォント: 今の2倍サイズ)
    // ==========================================
    _lblDate = lv_label_create(_panel);
    lv_label_set_text(_lblDate, "2026.08.29");
    lv_obj_set_style_text_color(_lblDate, COLOR_TEXT_PRIMARY, 0);
    lv_obj_set_style_text_font(_lblDate, &lv_font_montserrat_48, 0);
    lv_obj_align(_lblDate, LV_ALIGN_TOP_LEFT, 10, 40);

    _lblDayOfWeek = lv_label_create(_panel);
    lv_label_set_text(_lblDayOfWeek, "SAT");
    lv_obj_set_style_text_color(_lblDayOfWeek, COLOR_ACCENT_CYAN, 0);
    lv_obj_set_style_text_font(_lblDayOfWeek, &lv_font_montserrat_48, 0);
    lv_obj_align_to(_lblDayOfWeek, _lblDate, LV_ALIGN_OUT_RIGHT_MID, 20, 0);

    // ==========================================
    // 3. 領域いっぱいの超特大デジタル時計 (3x ズーム拡大キャンバス)
    // ==========================================
    size_t bufSize = LV_CANVAS_BUF_SIZE_TRUE_COLOR(CANVAS_WIDTH, CANVAS_HEIGHT);
    _canvasBuf = (lv_color_t*)heap_caps_malloc(bufSize, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (!_canvasBuf) {
        _canvasBuf = (lv_color_t*)malloc(bufSize);
    }

    _clockCanvas = lv_canvas_create(_panel);
    lv_canvas_set_buffer(_clockCanvas, _canvasBuf, CANVAS_WIDTH, CANVAS_HEIGHT, LV_IMG_CF_TRUE_COLOR);
    lv_canvas_fill_bg(_clockCanvas, COLOR_PANEL_BG, LV_OPA_COVER);
    lv_obj_align(_clockCanvas, LV_ALIGN_TOP_MID, 0, 140);
    lv_img_set_pivot(_clockCanvas, CANVAS_WIDTH / 2, CANVAS_HEIGHT / 2);
    lv_img_set_zoom(_clockCanvas, 680); // 約 2.7倍 (実効フォントサイズ約 130px 相当)

    // 初回描画
    lv_draw_label_dsc_t label_dsc;
    lv_draw_label_dsc_init(&label_dsc);
    label_dsc.color = COLOR_ACCENT_CYAN;
    label_dsc.font  = &lv_font_montserrat_48;
    label_dsc.align = LV_TEXT_ALIGN_CENTER;
    lv_canvas_draw_text(_clockCanvas, 0, 6, CANVAS_WIDTH, &label_dsc, "12:00:00");

    // ==========================================
    // 4. 音声対話・字幕エリア (日本語 TTF 18px)
    // ==========================================
    _panelSubtitle = lv_obj_create(_panel);
    lv_obj_set_size(_panelSubtitle, lv_pct(100), 120);
    lv_obj_align(_panelSubtitle, LV_ALIGN_BOTTOM_LEFT, 0, -68);
    lv_obj_set_style_bg_color(_panelSubtitle, lv_color_hex(0x181B22), 0);
    lv_obj_set_style_border_color(_panelSubtitle, COLOR_PANEL_BORDER, 0);
    lv_obj_set_style_border_width(_panelSubtitle, 1, 0);
    lv_obj_set_style_radius(_panelSubtitle, 12, 0);
    lv_obj_set_style_pad_all(_panelSubtitle, 10, 0);

    _lblUserVoice = lv_label_create(_panelSubtitle);
    lv_obj_set_style_text_font(_lblUserVoice, UITheme::font_body_18, 0);
    lv_label_set_text(_lblUserVoice, "🎤 「話しかけてください」");
    lv_obj_set_style_text_color(_lblUserVoice, COLOR_TEXT_SECONDARY, 0);
    lv_label_set_long_mode(_lblUserVoice, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(_lblUserVoice, lv_pct(100));
    lv_obj_align(_lblUserVoice, LV_ALIGN_TOP_LEFT, 0, 0);

    _lblAssistantReply = lv_label_create(_panelSubtitle);
    lv_obj_set_style_text_font(_lblAssistantReply, UITheme::font_body_18, 0);
    lv_label_set_text(_lblAssistantReply, "🤖 LLM Ready.");
    lv_obj_set_style_text_color(_lblAssistantReply, COLOR_ACCENT_CYAN, 0);
    lv_label_set_long_mode(_lblAssistantReply, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(_lblAssistantReply, lv_pct(100));
    lv_obj_align(_lblAssistantReply, LV_ALIGN_TOP_LEFT, 0, 42);

    // ==========================================
    // 5. アクションボタン (時刻読み上げ: 日本語 TTF 18px)
    // ==========================================
    _btnSpeakTime = lv_btn_create(_panel);
    lv_obj_set_size(_btnSpeakTime, lv_pct(100), 50);
    lv_obj_align(_btnSpeakTime, LV_ALIGN_BOTTOM_MID, 0, -6);
    lv_obj_add_style(_btnSpeakTime, &UITheme::style_btn_primary, 0);
    lv_obj_add_event_cb(_btnSpeakTime, btn_speak_event_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t* lblBtn = lv_label_create(_btnSpeakTime);
    lv_label_set_text(lblBtn, LV_SYMBOL_VOLUME_MAX " 現在時刻を音声で読み上げる");
    lv_obj_set_style_text_font(lblBtn, UITheme::font_body_18, 0);
    lv_obj_center(lblBtn);
}

void UIClock::updateTime(const FormattedDateTime& dt) {
    if (_lblDate) {
        lv_label_set_text(_lblDate, dt.dateStr);
    }
    if (_lblDayOfWeek) {
        // 曜日短縮形 (例: SATURDAY -> SAT)
        char dayShort[8] = {0};
        strncpy(dayShort, dt.dayOfWeekStr, 3);
        lv_label_set_text(_lblDayOfWeek, dayShort);
    }

    if (_clockCanvas) {
        lv_canvas_fill_bg(_clockCanvas, COLOR_PANEL_BG, LV_OPA_COVER);

        lv_draw_label_dsc_t label_dsc;
        lv_draw_label_dsc_init(&label_dsc);
        label_dsc.color = COLOR_ACCENT_CYAN;
        label_dsc.font  = &lv_font_montserrat_48;
        label_dsc.align = LV_TEXT_ALIGN_CENTER;

        lv_canvas_draw_text(_clockCanvas, 0, 6, CANVAS_WIDTH, &label_dsc, dt.timeStr);
    }
}

void UIClock::updateStatus(const SystemStatus& status) {
    if (_lblWifiStatus) {
        if (status.wifiConnected) {
            lv_label_set_text_fmt(_lblWifiStatus, LV_SYMBOL_WIFI " %s (%ddBm)", status.ipAddress.c_str(), status.wifiRssi);
            lv_obj_set_style_text_color(_lblWifiStatus, COLOR_STATUS_OK, 0);
        } else {
            lv_label_set_text(_lblWifiStatus, LV_SYMBOL_WIFI " Disconnected");
            lv_obj_set_style_text_color(_lblWifiStatus, COLOR_STATUS_ERR, 0);
        }
    }

    if (_lblLlmStatus) {
        if (status.llmModuleReady) {
            lv_label_set_text(_lblLlmStatus, "LLM: Online");
            lv_obj_set_style_text_color(_lblLlmStatus, COLOR_STATUS_OK, 0);
        } else {
            lv_label_set_text(_lblLlmStatus, "LLM: Standby");
            lv_obj_set_style_text_color(_lblLlmStatus, COLOR_STATUS_ERR, 0);
        }
    }

    if (_lblBattery) {
        lv_label_set_text_fmt(_lblBattery, "%s %d%%", 
            status.isCharging ? LV_SYMBOL_CHARGE : LV_SYMBOL_BATTERY_FULL, 
            status.batteryPercentage);
    }
}

void UIClock::setVoiceSubtitle(const String& userText, const String& replyText) {
    if (_lblUserVoice) {
        lv_label_set_text_fmt(_lblUserVoice, "🎤 %s", userText.c_str());
    }
    if (_lblAssistantReply) {
        lv_label_set_text_fmt(_lblAssistantReply, "🤖 %s", replyText.c_str());
    }
}

void UIClock::clearVoiceSubtitle() {
    if (_lblUserVoice) {
        lv_label_set_text(_lblUserVoice, "🎤 待機中...");
    }
    if (_lblAssistantReply) {
        lv_label_set_text(_lblAssistantReply, "🤖 何でも話しかけてください");
    }
}

void UIClock::handleSpeakTimeBtn() {
    if (_onSpeakTimeCallback) {
        _onSpeakTimeCallback();
    }
}
