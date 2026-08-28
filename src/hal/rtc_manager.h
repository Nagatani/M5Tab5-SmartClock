#pragma once

#include <Arduino.h>
#include <time.h>
#include <M5Unified.h>
#include "config.h"

struct FormattedDateTime {
    char timeStr[16];     // "16:49:05"
    char dateStr[32];     // "2026年08月28日"
    char dayOfWeekStr[16];// "金曜日"
    int hour;
    int minute;
    int second;
    int year;
    int month;
    int day;
    int dayOfWeek;        // 0=Sun, 1=Mon, ..., 6=Sat
};

class RTCManager {
public:
    static RTCManager& getInstance() {
        static RTCManager instance;
        return instance;
    }

    bool init();
    
    // Wi-Fi接続時にNTPから時刻取得し、内蔵RTC (RX8130CE) へ同期
    bool syncNTP();

    // 現在時刻の取得 (RTC / システムクロック)
    FormattedDateTime getCurrentDateTime();

    // TTS読み上げ用テキストの生成 (例: "現在の時刻は、午後4時49分です。")
    String getTTSDateTimeString();

private:
    RTCManager() = default;
    ~RTCManager() = default;

    bool _isRtcAvailable = false;
    unsigned long _lastNtpSyncMillis = 0;
};
