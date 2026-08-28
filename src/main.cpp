#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>

#include "config.h"
#include "types.h"
#include "hal/bsp_tab5.h"
#include "hal/rtc_manager.h"
#include "ui/ui_manager.h"
#include "network/wifi_manager.h"
#include "network/rss_parser.h"
#include "llm/llm_module_wrapper.h"
#include "llm/intent_dispatcher.h"

// ==========================================
// グローバル変数・同期オブジェクト
// ==========================================
static SystemStatus g_systemStatus;
static SemaphoreHandle_t g_newsMutex = nullptr;
static NewsCategoryData g_newsCategories[CAT_MAX];
static bool g_requestRssUpdate = true;

// ==========================================
// コールバック関数群
// ==========================================

// 時刻読み上げボタン押下時
void onSpeakTimeRequested() {
    String ttsText = RTCManager::getInstance().getTTSDateTimeString();
    Serial.printf("[Main] Speak Time Triggered: %s\n", ttsText.c_str());
    LLMModuleWrapper::getInstance().enqueueTTS(ttsText, true);

    if (UIManager::getInstance().lock(pdMS_TO_TICKS(100))) {
        UIClock::getInstance().setVoiceSubtitle("「今の時間を教えて」", ttsText);
        UIManager::getInstance().unlock();
    }
}

// ニュース読み上げボタン押下時
void onSpeakNewsRequested(const NewsItem& item) {
    String ttsText = "ニュースをお伝えします。" + item.title + "。" + item.description;
    Serial.printf("[Main] Speak News Triggered: %s\n", item.title.c_str());
    LLMModuleWrapper::getInstance().enqueueTTS(ttsText, false);

    if (UIManager::getInstance().lock(pdMS_TO_TICKS(100))) {
        UIClock::getInstance().setVoiceSubtitle("「ニュースを読んで」", "「" + item.title + "」を読み上げ中");
        UIManager::getInstance().unlock();
    }
}

// 音声認識イベント発生時 (ASR / KWS)
void onVoiceEventReceived(const VoiceEvent& ev) {
    Serial.printf("[Main] Voice Event Intent: %d, Text: %s\n", ev.intent, ev.recognizedText.c_str());

    String reply = "";
    NewsCategoryType targetCat = CAT_TOP;
    VoiceIntentType intent = IntentDispatcher::getInstance().parseIntent(ev.recognizedText, targetCat);

    switch (intent) {
        case INTENT_TELL_TIME: {
            reply = RTCManager::getInstance().getTTSDateTimeString();
            LLMModuleWrapper::getInstance().enqueueTTS(reply, true);
            break;
        }
        case INTENT_READ_NEWS: {
            const NewsItem* item = UINews::getInstance().getCurrentNewsItem();
            if (item) {
                reply = "「" + item->title + "」をお読みします。";
                LLMModuleWrapper::getInstance().enqueueTTS(reply + item->description, false);
            } else {
                reply = "現在表示できるニュースがありません。";
                LLMModuleWrapper::getInstance().enqueueTTS(reply, false);
            }
            break;
        }
        case INTENT_NEXT_NEWS: {
            reply = "次のニュースを表示します。";
            if (UIManager::getInstance().lock(pdMS_TO_TICKS(100))) {
                UINews::getInstance().nextNews();
                UIManager::getInstance().unlock();
            }
            break;
        }
        case INTENT_PREV_NEWS: {
            reply = "前のニュースを表示します。";
            if (UIManager::getInstance().lock(pdMS_TO_TICKS(100))) {
                UINews::getInstance().prevNews();
                UIManager::getInstance().unlock();
            }
            break;
        }
        case INTENT_CHANGE_CATEGORY: {
            reply = "カテゴリを切り替えました。";
            if (UIManager::getInstance().lock(pdMS_TO_TICKS(100))) {
                UINews::getInstance().selectCategory(targetCat);
                UIManager::getInstance().unlock();
            }
            break;
        }
        default: {
            reply = "承知いたしました。";
            break;
        }
    }

    if (UIManager::getInstance().lock(pdMS_TO_TICKS(100))) {
        UIClock::getInstance().setVoiceSubtitle(ev.recognizedText, reply);
        UIManager::getInstance().unlock();
    }
}

// ==========================================
// FreeRTOS タスク定義
// ==========================================

// 1. GUI 描画 & タイマータスク (Core 1)
void Task_GUI(void* pvParameters) {
    TickType_t lastClockUpdate = 0;
    TickType_t lastSlideUpdate = 0;

    while (1) {
        UIManager::getInstance().loop();

        TickType_t now = xTaskGetTickCount();

        // 1秒ごとの時計・ステータス更新
        if ((now - lastClockUpdate) >= pdMS_TO_TICKS(1000)) {
            lastClockUpdate = now;
            FormattedDateTime dt = RTCManager::getInstance().getCurrentDateTime();

            if (UIManager::getInstance().lock(pdMS_TO_TICKS(50))) {
                UIClock::getInstance().updateTime(dt);
                UIClock::getInstance().updateStatus(g_systemStatus);
                UIManager::getInstance().unlock();
            }
        }

        // 10秒ごとのニュース自動スライド
        if ((now - lastSlideUpdate) >= pdMS_TO_TICKS(NEWS_AUTO_SLIDE_SEC * 1000)) {
            lastSlideUpdate = now;
            if (UIManager::getInstance().lock(pdMS_TO_TICKS(50))) {
                UINews::getInstance().nextNews();
                UIManager::getInstance().unlock();
            }
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

// 2. ネットワーク & RSS パースタスク (Core 0)
void Task_Network(void* pvParameters) {
    WiFiManager::getInstance().connect();
    
    if (WiFiManager::getInstance().isConnected()) {
        RTCManager::getInstance().syncNTP();
        g_systemStatus.ntpSynced = true;
    }

    TickType_t lastRssFetch = 0;
    TickType_t lastNtpSync = xTaskGetTickCount();

    while (1) {
        // Wi-Fi 状態更新 & 切断時の自動再接続
        WiFiManager::getInstance().checkAndReconnect();
        WiFiManager::getInstance().updateStatus(g_systemStatus);
        BSPTab5::getInstance().updatePowerStatus(g_systemStatus);

        TickType_t now = xTaskGetTickCount();

        // 24時間ごとの NTP 再同期
        if ((now - lastNtpSync) >= pdMS_TO_TICKS(NTP_SYNC_INTERVAL_MS)) {
            if (WiFiManager::getInstance().isConnected()) {
                RTCManager::getInstance().syncNTP();
                lastNtpSync = now;
            }
        }

        // RSS フィードの定期フェッチ (5分ごと、または要求時)
        if (g_requestRssUpdate || (now - lastRssFetch) >= pdMS_TO_TICKS(RSS_FETCH_INTERVAL_MS)) {
            if (WiFiManager::getInstance().isConnected()) {
                Serial.println("[NetTask] Fetching all RSS categories...");
                for (int i = 0; i < CAT_MAX; ++i) {
                    NewsCategoryData tempCat;
                    if (RSSParser::getInstance().fetchCategory((NewsCategoryType)i, tempCat)) {
                        if (xSemaphoreTake(g_newsMutex, portMAX_DELAY) == pdTRUE) {
                            g_newsCategories[i] = tempCat;
                            xSemaphoreGive(g_newsMutex);
                        }

                        if (UIManager::getInstance().lock(pdMS_TO_TICKS(100))) {
                            UINews::getInstance().updateCategoryData((NewsCategoryType)i, tempCat);
                            UIManager::getInstance().unlock();
                        }
                    }
                    vTaskDelay(pdMS_TO_TICKS(300));
                }
                lastRssFetch = now;
                g_requestRssUpdate = false;
            }
        }

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

// 3. LLM Module 通信 & 音声制御タスク (Core 0)
void Task_LLM_Com(void* pvParameters) {
    LLMModuleWrapper::getInstance().init();
    LLMModuleWrapper::getInstance().setOnVoiceEventCallback(onVoiceEventReceived);

    while (1) {
        LLMModuleWrapper::getInstance().update();
        LLMModuleWrapper::getInstance().updateStatus(g_systemStatus);
        vTaskDelay(pdMS_TO_TICKS(20));
    }
}

// ==========================================
// setup & loop
// ==========================================
void setup() {
    Serial.begin(115200);
    Serial.println("\n=== M5Stack Tab5 Smart Clock & News Starting ===");

    g_newsMutex = xSemaphoreCreateMutex();

    // 1. ハードウェア & RTC 初期化
    BSPTab5::getInstance().init();
    RTCManager::getInstance().init();
    g_systemStatus.rtcOk = true;

    // 2. LVGL UI 初期化
    UIManager::getInstance().init();

    // 3. UI コールバック登録
    UIClock::getInstance().setOnSpeakTimeCallback(onSpeakTimeRequested);
    UINews::getInstance().setOnSpeakNewsCallback(onSpeakNewsRequested);

    // 4. FreeRTOS タスク生成
    xTaskCreatePinnedToCore(
        Task_GUI,
        "Task_GUI",
        TASK_GUI_STACK_SIZE,
        NULL,
        TASK_GUI_PRIORITY,
        NULL,
        TASK_GUI_CORE
    );

    xTaskCreatePinnedToCore(
        Task_Network,
        "Task_Network",
        TASK_NET_STACK_SIZE,
        NULL,
        TASK_NET_PRIORITY,
        NULL,
        TASK_NET_CORE
    );

    xTaskCreatePinnedToCore(
        Task_LLM_Com,
        "Task_LLM_Com",
        TASK_LLM_STACK_SIZE,
        NULL,
        TASK_LLM_PRIORITY,
        NULL,
        TASK_LLM_CORE
    );

    Serial.println("[Main] All tasks started successfully.");
}

void loop() {
    // FreeRTOS タスク駆動のため Arduino メインループは休止
    vTaskDelay(pdMS_TO_TICKS(1000));
}
