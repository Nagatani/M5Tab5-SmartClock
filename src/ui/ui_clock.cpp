#include "ui_clock.h"
#include "ui_theme.h"
#include "config.h"
#include <esp_heap_caps.h>

#define CANVAS_WIDTH  360
#define CANVAS_HEIGHT 65

static void btn_speak_event_cb(lv_event_t* e) {
    if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
        UIClock::getInstance().handleSpeakTimeBtn();
    }
}

void UIClock::init(lv_obj_t* parent) {
    // ====================================================================
    // 1. 上部メイン時計パネル (画面横幅いっぱい: 1260 x 405)
    // ====================================================================
    _panelClock = lv_obj_create(parent);
    lv_obj_set_pos(_panelClock, 10, 10);
    lv_obj_set_size(_panelClock, SCREEN_WIDTH - 20, CLOCK_PANEL_HEIGHT - 10);
    lv_obj_add_style(_panelClock, &UITheme::style_panel, 0);
    lv_obj_clear_flag(_panelClock, LV_OBJ_FLAG_SCROLLABLE);

    // --- ステータスバー (上部 y: 0〜28) ---
    lv_obj_t* statusBar = lv_obj_create(_panelClock);
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

    // --- 日付・曜日表示 (48px Montserrat) ---
    _lblDate = lv_label_create(_panelClock);
    lv_label_set_text(_lblDate, "2026.08.29");
    lv_obj_set_style_text_color(_lblDate, COLOR_TEXT_PRIMARY, 0);
    lv_obj_set_style_text_font(_lblDate, &lv_font_montserrat_48, 0);
    lv_obj_align(_lblDate, LV_ALIGN_TOP_LEFT, 15, 36);

    _lblDayOfWeek = lv_label_create(_panelClock);
    lv_label_set_text(_lblDayOfWeek, "SATURDAY");
    lv_obj_set_style_text_color(_lblDayOfWeek, COLOR_ACCENT_CYAN, 0);
    lv_obj_set_style_text_font(_lblDayOfWeek, &lv_font_montserrat_48, 0);
    lv_obj_align_to(_lblDayOfWeek, _lblDate, LV_ALIGN_OUT_RIGHT_MID, 25, 0);

    // --- 横幅いっぱいの超特大デジタル時計 (ズーム拡大キャンバス) ---
    size_t bufSize = LV_CANVAS_BUF_SIZE_TRUE_COLOR(CANVAS_WIDTH, CANVAS_HEIGHT);
    _canvasBuf = (lv_color_t*)heap_caps_malloc(bufSize, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (!_canvasBuf) {
        _canvasBuf = (lv_color_t*)malloc(bufSize);
    }

    _clockCanvas = lv_canvas_create(_panelClock);
    lv_canvas_set_buffer(_clockCanvas, _canvasBuf, CANVAS_WIDTH, CANVAS_HEIGHT, LV_IMG_CF_TRUE_COLOR);
    lv_canvas_fill_bg(_clockCanvas, COLOR_PANEL_BG, LV_OPA_COVER);
    lv_obj_align(_clockCanvas, LV_ALIGN_TOP_MID, 0, 160);
    lv_img_set_pivot(_clockCanvas, CANVAS_WIDTH / 2, CANVAS_HEIGHT / 2);
    lv_img_set_zoom(_clockCanvas, 850); // 約 3.3倍 (実効フォントサイズ約 160px〜180px 相当！幅約 950px)

    // 初回描画
    lv_draw_label_dsc_t label_dsc;
    lv_draw_label_dsc_init(&label_dsc);
    label_dsc.color = COLOR_ACCENT_CYAN;
    label_dsc.font  = &lv_font_montserrat_48;
    label_dsc.align = LV_TEXT_ALIGN_CENTER;
    lv_canvas_draw_text(_clockCanvas, 0, 8, CANVAS_WIDTH, &label_dsc, "12:00:00");

    // ====================================================================
    // 2. 下部右側：LLM 音声対話 ＆ アクションパネル (625 x 290)
    // ====================================================================
    _panelLlm = lv_obj_create(parent);
    lv_obj_set_pos(_panelLlm, SCREEN_WIDTH - BOTTOM_PANEL_WIDTH - 10, CLOCK_PANEL_HEIGHT + 15);
    lv_obj_set_size(_panelLlm, BOTTOM_PANEL_WIDTH, BOTTOM_PANEL_HEIGHT - 5);
    lv_obj_add_style(_panelLlm, &UITheme::style_panel, 0);
    lv_obj_clear_flag(_panelLlm, LV_OBJ_FLAG_SCROLLABLE);

    // ヘッダータイトル
    lv_obj_t* lblLlmTitle = lv_label_create(_panelLlm);
    lv_label_set_text(lblLlmTitle, "🤖 AI 音声アシスタント");
    lv_obj_set_style_text_font(lblLlmTitle, UITheme::font_title_24, 0);
    lv_obj_set_style_text_color(lblLlmTitle, COLOR_ACCENT_CYAN, 0);
    lv_obj_align(lblLlmTitle, LV_ALIGN_TOP_LEFT, 0, 0);

    // 字幕ボックス (高さ 135px)
    lv_obj_t* subBox = lv_obj_create(_panelLlm);
    lv_obj_set_size(subBox, lv_pct(100), 135);
    lv_obj_align(subBox, LV_ALIGN_TOP_LEFT, 0, 36);
    lv_obj_set_style_bg_color(subBox, lv_color_hex(0x181B22), 0);
    lv_obj_set_style_border_color(subBox, COLOR_PANEL_BORDER, 0);
    lv_obj_set_style_border_width(subBox, 1, 0);
    lv_obj_set_style_radius(subBox, 10, 0);
    lv_obj_set_style_pad_all(subBox, 8, 0);

    _lblUserVoice = lv_label_create(subBox);
    lv_obj_set_style_text_font(_lblUserVoice, UITheme::font_body_18, 0);
    lv_label_set_text(_lblUserVoice, "🎤 「話しかけてください (例: 今日のニュース)」");
    lv_obj_set_style_text_color(_lblUserVoice, COLOR_TEXT_SECONDARY, 0);
    lv_label_set_long_mode(_lblUserVoice, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(_lblUserVoice, lv_pct(100));
    lv_obj_align(_lblUserVoice, LV_ALIGN_TOP_LEFT, 0, 0);

    _lblAssistantReply = lv_label_create(subBox);
    lv_obj_set_style_text_font(_lblAssistantReply, UITheme::font_body_18, 0);
    lv_label_set_text(_lblAssistantReply, "🤖 LLM Ready. 音声コマンド待機中");
    lv_obj_set_style_text_color(_lblAssistantReply, COLOR_ACCENT_CYAN, 0);
    lv_label_set_long_mode(_lblAssistantReply, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(_lblAssistantReply, lv_pct(100));
    lv_obj_align(_lblAssistantReply, LV_ALIGN_TOP_LEFT, 0, 50);

    // 時刻読み上げアクションボタン (高さ 48px)
    _btnSpeakTime = lv_btn_create(_panelLlm);
    lv_obj_set_size(_btnSpeakTime, lv_pct(100), 48);
    lv_obj_align(_btnSpeakTime, LV_ALIGN_BOTTOM_MID, 0, 0);
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
        lv_label_set_text(_lblDayOfWeek, dt.dayOfWeekStr);
    }

    if (_clockCanvas) {
        lv_canvas_fill_bg(_clockCanvas, COLOR_PANEL_BG, LV_OPA_COVER);

        lv_draw_label_dsc_t label_dsc;
        lv_draw_label_dsc_init(&label_dsc);
        label_dsc.color = COLOR_ACCENT_CYAN;
        label_dsc.font  = &lv_font_montserrat_48;
        label_dsc.align = LV_TEXT_ALIGN_CENTER;

        lv_canvas_draw_text(_clockCanvas, 0, 8, CANVAS_WIDTH, &label_dsc, dt.timeStr);
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
