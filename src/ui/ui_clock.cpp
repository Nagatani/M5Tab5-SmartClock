#include "ui_clock.h"
#include "ui_theme.h"

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
    // 1. ステータスバー (上部 y: 0〜32)
    // ==========================================
    lv_obj_t* statusBar = lv_obj_create(_panel);
    lv_obj_set_size(statusBar, lv_pct(100), 32);
    lv_obj_set_pos(statusBar, 0, 0);
    lv_obj_set_style_bg_opa(statusBar, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(statusBar, 0, 0);
    lv_obj_set_style_pad_all(statusBar, 0, 0);
    lv_obj_set_flex_flow(statusBar, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(statusBar, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    _lblWifiStatus = lv_label_create(statusBar);
    lv_obj_set_style_text_font(_lblWifiStatus, UITheme::font_small_14, 0);
    lv_label_set_text(_lblWifiStatus, LV_SYMBOL_WIFI " 接続中...");
    lv_obj_set_style_text_color(_lblWifiStatus, COLOR_TEXT_SECONDARY, 0);

    _lblRtcStatus = lv_label_create(statusBar);
    lv_obj_set_style_text_font(_lblRtcStatus, UITheme::font_small_14, 0);
    lv_label_set_text(_lblRtcStatus, "RTC: OK");
    lv_obj_set_style_text_color(_lblRtcStatus, COLOR_STATUS_OK, 0);

    _lblLlmStatus = lv_label_create(statusBar);
    lv_obj_set_style_text_font(_lblLlmStatus, UITheme::font_small_14, 0);
    lv_label_set_text(_lblLlmStatus, "LLM: 待機中");
    lv_obj_set_style_text_color(_lblLlmStatus, COLOR_TEXT_SECONDARY, 0);

    _lblBattery = lv_label_create(statusBar);
    lv_obj_set_style_text_font(_lblBattery, UITheme::font_small_14, 0);
    lv_label_set_text(_lblBattery, LV_SYMBOL_BATTERY_FULL " 100%");
    lv_obj_set_style_text_color(_lblBattery, COLOR_TEXT_SECONDARY, 0);

    // ==========================================
    // 2. 日付・曜日表示 (拡大: 28px フォント)
    // ==========================================
    _lblDate = lv_label_create(_panel);
    lv_label_set_text(_lblDate, "2026年08月29日");
    lv_obj_set_style_text_color(_lblDate, COLOR_TEXT_PRIMARY, 0);
    lv_obj_set_style_text_font(_lblDate, UITheme::font_date_28, 0);
    lv_obj_align(_lblDate, LV_ALIGN_TOP_LEFT, 10, 48);

    _lblDayOfWeek = lv_label_create(_panel);
    lv_label_set_text(_lblDayOfWeek, "(土曜日)");
    lv_obj_set_style_text_color(_lblDayOfWeek, COLOR_ACCENT_CYAN, 0);
    lv_obj_set_style_text_font(_lblDayOfWeek, UITheme::font_date_28, 0);
    lv_obj_align_to(_lblDayOfWeek, _lblDate, LV_ALIGN_OUT_RIGHT_MID, 16, 0);

    // ==========================================
    // 3. 特大デジタル時計表示 (64px 特大フォント)
    // ==========================================
    _lblTime = lv_label_create(_panel);
    lv_label_set_text(_lblTime, "12:00:00");
    lv_obj_set_style_text_color(_lblTime, COLOR_ACCENT_CYAN, 0);
    lv_obj_set_style_text_font(_lblTime, UITheme::font_clock_64, 0);
    lv_obj_align(_lblTime, LV_ALIGN_TOP_LEFT, 10, 100);

    // ==========================================
    // 4. 音声対話・字幕エリア (コンパクト化: 高さ 140px)
    // ==========================================
    _panelSubtitle = lv_obj_create(_panel);
    lv_obj_set_size(_panelSubtitle, lv_pct(100), 140);
    lv_obj_align(_panelSubtitle, LV_ALIGN_BOTTOM_LEFT, 0, -80);
    lv_obj_set_style_bg_color(_panelSubtitle, lv_color_hex(0x181B22), 0);
    lv_obj_set_style_border_color(_panelSubtitle, COLOR_PANEL_BORDER, 0);
    lv_obj_set_style_border_width(_panelSubtitle, 1, 0);
    lv_obj_set_style_radius(_panelSubtitle, 12, 0);
    lv_obj_set_style_pad_all(_panelSubtitle, 12, 0);

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
    lv_obj_align(_lblAssistantReply, LV_ALIGN_TOP_LEFT, 0, 48);

    // ==========================================
    // 5. アクションボタン (時刻読み上げ: 高さ 55px)
    // ==========================================
    _btnSpeakTime = lv_btn_create(_panel);
    lv_obj_set_size(_btnSpeakTime, lv_pct(100), 55);
    lv_obj_align(_btnSpeakTime, LV_ALIGN_BOTTOM_MID, 0, -8);
    lv_obj_add_style(_btnSpeakTime, &UITheme::style_btn_primary, 0);
    lv_obj_add_event_cb(_btnSpeakTime, btn_speak_event_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t* lblBtn = lv_label_create(_btnSpeakTime);
    lv_label_set_text(lblBtn, LV_SYMBOL_VOLUME_MAX " 現在時刻を音声で読み上げる");
    lv_obj_set_style_text_font(lblBtn, UITheme::font_body_18, 0);
    lv_obj_center(lblBtn);
}

void UIClock::updateTime(const FormattedDateTime& dt) {
    if (!_lblTime) return;
    lv_label_set_text(_lblTime, dt.timeStr);
    lv_label_set_text(_lblDate, dt.dateStr);
    lv_label_set_text_fmt(_lblDayOfWeek, "(%s)", dt.dayOfWeekStr);
}

void UIClock::updateStatus(const SystemStatus& status) {
    if (_lblWifiStatus) {
        if (status.wifiConnected) {
            lv_label_set_text_fmt(_lblWifiStatus, LV_SYMBOL_WIFI " %s (%ddBm)", status.ipAddress.c_str(), status.wifiRssi);
            lv_obj_set_style_text_color(_lblWifiStatus, COLOR_STATUS_OK, 0);
        } else {
            lv_label_set_text(_lblWifiStatus, LV_SYMBOL_WIFI " 未接続");
            lv_obj_set_style_text_color(_lblWifiStatus, COLOR_STATUS_ERR, 0);
        }
    }

    if (_lblLlmStatus) {
        if (status.llmModuleReady) {
            lv_label_set_text(_lblLlmStatus, "LLM: オンライン");
            lv_obj_set_style_text_color(_lblLlmStatus, COLOR_STATUS_OK, 0);
        } else {
            lv_label_set_text(_lblLlmStatus, "LLM: 未接続");
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
