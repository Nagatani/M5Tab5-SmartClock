#pragma once

#include <Arduino.h>
#include "config.h"

// ==========================================
// 1. News Data Types
// ==========================================
enum NewsCategoryType {
    CAT_TOP = 0,
    CAT_IT,
    CAT_BUSINESS,
    CAT_WORLD,
    CAT_MAX
};

struct NewsItem {
    String title;
    String description;
    String pubDate;
    String link;
};

struct NewsCategoryData {
    String name;
    String rssUrl;
    NewsItem items[MAX_NEWS_ITEMS_PER_CAT];
    size_t itemCount = 0;
    String lastUpdated;
    bool isLoaded = false;
};

// ==========================================
// 2. Voice & LLM Event Types
// ==========================================
enum VoiceIntentType {
    INTENT_NONE = 0,
    INTENT_TELL_TIME,       // 「今の時間を教えて」「何時？」
    INTENT_READ_NEWS,       // 「ニュースを読んで」「詳細を教えて」
    INTENT_NEXT_NEWS,       // 「次のニュース」「次へ」
    INTENT_PREV_NEWS,       // 「前のニュース」「戻る」
    INTENT_CHANGE_CATEGORY, // 「ITニュースにして」「経済に変えて」
    INTENT_FREE_CHAT        // その他の自由対話
};

struct VoiceEvent {
    VoiceIntentType intent;
    String recognizedText;
    String assistantReply;
    uint32_t timestamp;
};

struct TTSCommand {
    String text;
    bool highPriority;
};

// ==========================================
// 3. System Status
// ==========================================
struct SystemStatus {
    bool wifiConnected = false;
    int8_t wifiRssi = 0;
    String ipAddress = "";
    bool ntpSynced = false;
    bool rtcOk = false;
    bool llmModuleReady = false;
    int batteryPercentage = 100;
    bool isCharging = false;
};
