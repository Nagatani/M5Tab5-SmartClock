#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>
#include <esp_heap_caps.h>

#include "config.h"
#include "types.h"
#include "hal/bsp_tab5.h"
#include "hal/rtc_manager.h"
#include "ui/ui_manager.h"
#include "network/wifi_manager.h"
#include "network/rss_parser.h"

// ==========================================
// グローバル変数・同期オブジェクト
// ==========================================
static SystemStatus g_systemStatus;
static SemaphoreHandle_t g_newsMutex = nullptr;
static NewsCategoryData g_newsCategories[CAT_MAX];
static bool g_requestFullRssUpdate = true;
static int g_requestSingleCatFetch = -1;

// PSRAM タスクスタック用バッファ
static StaticTask_t g_taskGuiBuffer;
static StackType_t* g_taskGuiStack = nullptr;

static StaticTask_t g_taskNetBuffer;
static StackType_t* g_taskNetStack = nullptr;

static StaticTask_t g_taskLlmBuffer;
static StackType_t* g_taskLlmStack = nullptr;

// ==========================================
// コールバック関数群
// ==========================================

// 時刻読み上げボタン押下時
void onSpeakTimeRequested() {

}

// ニュース読み上げボタン押下時
void onSpeakNewsRequested(const NewsItem& item) {

}

// カテゴリ切替時 (UI / 音声)
void onCategoryChanged(NewsCategoryType catType) {
    Serial.printf("[Main] Category Switched to: %d\n", (int)catType);
    if (!g_newsCategories[catType].isLoaded) {
        g_requestSingleCatFetch = (int)catType;
    }
}

// 音声認識イベント発生時 (ASR / KWS)
void onVoiceEventReceived(const VoiceEvent& ev) {

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

// 2. ネットワーク & RSS パースタスク (Core 0, Priority 1)
void Task_Network(void* pvParameters) {
    WiFiManager::getInstance().connect();
    
    if (WiFiManager::getInstance().isConnected()) {
        RTCManager::getInstance().syncNTP();
        g_systemStatus.ntpSynced = true;
    }

    TickType_t lastNtpSync = xTaskGetTickCount();
    TickType_t lastFullRssFetch = 0;

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

        // 1時間に1回の全カテゴリ定期取得 (または起動時)
        if (WiFiManager::getInstance().isConnected()) {
            if (g_requestFullRssUpdate || (now - lastFullRssFetch) >= pdMS_TO_TICKS(RSS_FETCH_INTERVAL_MS)) {
                Serial.println("\n[NetTask] Fetching RSS feeds (1 hour interval)...");
                for (int i = 0; i < CAT_MAX; ++i) {
                    NewsCategoryData tempCat;
                    if (RSSParser::getInstance().fetchCategory((NewsCategoryType)i, tempCat)) {
                        if (xSemaphoreTake(g_newsMutex, pdMS_TO_TICKS(500)) == pdTRUE) {
                            g_newsCategories[i] = tempCat;
                            xSemaphoreGive(g_newsMutex);
                        }

                        if (UIManager::getInstance().lock(pdMS_TO_TICKS(100))) {
                            UINews::getInstance().updateCategoryData((NewsCategoryType)i, tempCat);
                            UIManager::getInstance().unlock();
                        }
                    }
                    // ソケットクローズと DMA メモリ完全解放のための 3 秒インターバル
                    vTaskDelay(pdMS_TO_TICKS(3000));
                }
                lastFullRssFetch = xTaskGetTickCount();
                g_requestFullRssUpdate = false;
                Serial.println("[NetTask] Hourly RSS fetch completed. Next update in 1 hour.");
            }
            // 個別カテゴリの単発要求
            else if (g_requestSingleCatFetch >= 0 && g_requestSingleCatFetch < CAT_MAX) {
                int catIdx = g_requestSingleCatFetch;
                g_requestSingleCatFetch = -1;
                NewsCategoryData tempCat;
                if (RSSParser::getInstance().fetchCategory((NewsCategoryType)catIdx, tempCat)) {
                    if (xSemaphoreTake(g_newsMutex, pdMS_TO_TICKS(500)) == pdTRUE) {
                        g_newsCategories[catIdx] = tempCat;
                        xSemaphoreGive(g_newsMutex);
                    }
                    if (UIManager::getInstance().lock(pdMS_TO_TICKS(100))) {
                        UINews::getInstance().updateCategoryData((NewsCategoryType)catIdx, tempCat);
                        UIManager::getInstance().unlock();
                    }
                }
            }
        }

        vTaskDelay(pdMS_TO_TICKS(1000));
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
    UINews::getInstance().setOnCategoryChangeCallback(onCategoryChanged);

    // 4. FreeRTOS タスク生成 (スタックを PSRAM に割り当て)
    g_taskGuiStack = (StackType_t*)heap_caps_malloc(TASK_GUI_STACK_SIZE, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    g_taskNetStack = (StackType_t*)heap_caps_malloc(TASK_NET_STACK_SIZE, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    g_taskLlmStack = (StackType_t*)heap_caps_malloc(TASK_LLM_STACK_SIZE, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);

    if (g_taskGuiStack && g_taskNetStack && g_taskLlmStack) {
        xTaskCreateStaticPinnedToCore(
            Task_GUI, "Task_GUI", TASK_GUI_STACK_SIZE, NULL, TASK_GUI_PRIORITY,
            g_taskGuiStack, &g_taskGuiBuffer, TASK_GUI_CORE
        );

        xTaskCreateStaticPinnedToCore(
            Task_Network, "Task_Network", TASK_NET_STACK_SIZE, NULL, 1,
            g_taskNetStack, &g_taskNetBuffer, TASK_NET_CORE
        );

        Serial.println("[Main] All tasks started in PSRAM successfully.");
    } else {
        // フォールバック
        xTaskCreatePinnedToCore(Task_GUI, "Task_GUI", TASK_GUI_STACK_SIZE, NULL, TASK_GUI_PRIORITY, NULL, TASK_GUI_CORE);
        xTaskCreatePinnedToCore(Task_Network, "Task_Network", TASK_NET_STACK_SIZE, NULL, 1, NULL, TASK_NET_CORE);
        Serial.println("[Main] Tasks started in Internal SRAM (Fallback).");
    }

    Serial.printf("[Main] Free Internal Heap: %u bytes, Free PSRAM: %u bytes\n", 
                  (unsigned int)heap_caps_get_free_size(MALLOC_CAP_INTERNAL), 
                  (unsigned int)ESP.getFreePsram());
}

void loop() {
    vTaskDelay(pdMS_TO_TICKS(1000));
}
