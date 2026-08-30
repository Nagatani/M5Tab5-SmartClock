#include "rtc_manager.h"
#include <sys/time.h>

static const char* DAY_NAMES_JA[] = {
    "日曜日", "月曜日", "火曜日", "水曜日", "木曜日", "金曜日", "土曜日"
};

static const char* DAY_NAMES_EN[] = {
    "SUNDAY", "MONDAY", "TUESDAY", "WEDNESDAY", "THURSDAY", "FRIDAY", "SATURDAY"
};

bool RTCManager::init() {
    // M5Unified 内蔵 RTC の初期化確認
    if (M5.Rtc.isEnabled()) {
        _isRtcAvailable = true;
        Serial.println("[RTC] Built-in RX8130CE RTC detected.");

        // RTCからシステムクロックへ時刻ロード
        auto dt = M5.Rtc.getDateTime();
        struct tm t = {0};
        t.tm_year = dt.date.year - 1900;
        t.tm_mon  = dt.date.month - 1;
        t.tm_mday = dt.date.date;
        t.tm_hour = dt.time.hours;
        t.tm_min  = dt.time.minutes;
        t.tm_sec  = dt.time.seconds;
        time_t timeSinceEpoch = mktime(&t);
        
        struct timeval tv = { .tv_sec = timeSinceEpoch, .tv_usec = 0 };
        settimeofday(&tv, NULL);
    } else {
        Serial.println("[RTC] Warning: RTC not detected. Using internal timer.");
    }

    return true;
}

bool RTCManager::syncNTP() {
    Serial.println("[RTC] Synchronizing time with NTP (JST)...");
    configTime(TIME_ZONE_OFFSET_SEC, DAYLIGHT_OFFSET_SEC, NTP_SERVER1, NTP_SERVER2, NTP_SERVER3);

    struct tm timeinfo;
    int retry = 0;
    while (!getLocalTime(&timeinfo) && retry < 10) {
        Serial.print(".");
        delay(500);
        retry++;
    }

    if (retry >= 10) {
        Serial.println("\n[RTC] NTP synchronization failed.");
        return false;
    }

    Serial.println("\n[RTC] NTP synchronization successful!");

    // M5.Rtc へ時刻書き込み
    if (_isRtcAvailable) {
        m5::rtc_datetime_t dt;
        dt.date.year    = timeinfo.tm_year + 1900;
        dt.date.month   = timeinfo.tm_mon + 1;
        dt.date.date    = timeinfo.tm_mday;
        dt.date.weekDay = timeinfo.tm_wday;
        dt.time.hours   = timeinfo.tm_hour;
        dt.time.minutes = timeinfo.tm_min;
        dt.time.seconds = timeinfo.tm_sec;

        M5.Rtc.setDateTime(dt);
        Serial.println("[RTC] Synced RTC hardware with NTP time.");
    }

    _lastNtpSyncMillis = millis();
    return true;
}

FormattedDateTime RTCManager::getCurrentDateTime() {
    FormattedDateTime result;
    memset(&result, 0, sizeof(result));

    struct tm timeinfo;
    if (getLocalTime(&timeinfo)) {
        result.year = timeinfo.tm_year + 1900;
        result.month = timeinfo.tm_mon + 1;
        result.day = timeinfo.tm_mday;
        result.hour = timeinfo.tm_hour;
        result.minute = timeinfo.tm_min;
        result.second = timeinfo.tm_sec;
        result.dayOfWeek = timeinfo.tm_wday;

        snprintf(result.timeStr, sizeof(result.timeStr), "%02d:%02d:%02d", result.hour, result.minute, result.second);
        snprintf(result.dateStr, sizeof(result.dateStr), "%04d.%02d.%02d", result.year, result.month, result.day);
        snprintf(result.dayOfWeekStr, sizeof(result.dayOfWeekStr), "%s", DAY_NAMES_EN[result.dayOfWeek % 7]);
    } else {
        snprintf(result.timeStr, sizeof(result.timeStr), "--:--:--");
        snprintf(result.dateStr, sizeof(result.dateStr), "----.--.--");
        snprintf(result.dayOfWeekStr, sizeof(result.dayOfWeekStr), "---");
    }

    return result;
}

String RTCManager::getTTSDateTimeString() {
    FormattedDateTime dt = getCurrentDateTime();
    String ampm = (dt.hour < 12) ? "午前" : "午後";
    int hour12 = dt.hour % 12;
    if (hour12 == 0) hour12 = 12;

    // TTSの高速生成のため、短く明瞭な日本語にする
    char buffer[64];
    snprintf(buffer, sizeof(buffer), "%s%d時%d分です。", ampm.c_str(), hour12, dt.minute);

    return String(buffer);
}
