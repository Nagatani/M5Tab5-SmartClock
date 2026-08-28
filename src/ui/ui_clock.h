#pragma once

#include <Arduino.h>
#include <lvgl.h>
#include "types.h"
#include "hal/rtc_manager.h"

class UIClock {
public:
    static UIClock& getInstance() {
        static UIClock instance;
        return instance;
    }

    void init(lv_obj_t* parent);
    void updateTime(const FormattedDateTime& dt);
    void updateStatus(const SystemStatus& status);
    void setVoiceSubtitle(const String& userText, const String& replyText);
    void clearVoiceSubtitle();

    // コールバック関数ポインタ (時刻読み上げボタン押下時)
    void setOnSpeakTimeCallback(void (*callback)()) {
        _onSpeakTimeCallback = callback;
    }

    void handleSpeakTimeBtn();

private:
    UIClock() = default;
    ~UIClock() = default;

    lv_obj_t* _panel = nullptr;
    lv_obj_t* _lblTime = nullptr;
    lv_obj_t* _lblDate = nullptr;
    lv_obj_t* _lblDayOfWeek = nullptr;
    
    // Status Bar Widgets
    lv_obj_t* _lblWifiStatus = nullptr;
    lv_obj_t* _lblRtcStatus = nullptr;
    lv_obj_t* _lblLlmStatus = nullptr;
    lv_obj_t* _lblBattery = nullptr;

    // Subtitle Widgets
    lv_obj_t* _panelSubtitle = nullptr;
    lv_obj_t* _lblUserVoice = nullptr;
    lv_obj_t* _lblAssistantReply = nullptr;

    // Speak Button
    lv_obj_t* _btnSpeakTime = nullptr;

    void (*_onSpeakTimeCallback)() = nullptr;
};
