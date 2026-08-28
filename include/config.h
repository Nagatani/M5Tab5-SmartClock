#pragma once

#include <Arduino.h>

// ==========================================
// 1. Wi-Fi & Network Settings
// ==========================================
#define WIFI_SSID               "YOUR_WIFI_SSID"
#define WIFI_PASSWORD           "YOUR_WIFI_PASSWORD"
#define WIFI_CONNECT_TIMEOUT_MS 15000

// NTP Server Settings (JST: UTC+9)
#define NTP_SERVER1             "ntp.nict.jp"
#define NTP_SERVER2             "time.google.com"
#define NTP_SERVER3             "pool.ntp.org"
#define TIME_ZONE_OFFSET_SEC    (9 * 3600)  // UTC+9
#define DAYLIGHT_OFFSET_SEC     0
#define NTP_SYNC_INTERVAL_MS    (24 * 3600 * 1000) // 1日1回同期

// ==========================================
// 2. Hardware & Pin Definitions (M5Stack Tab5)
// ==========================================
#define SCREEN_WIDTH            1280
#define SCREEN_HEIGHT           720

// Display Split Layout
#define CLOCK_PANEL_WIDTH       704   // 左側 55%
#define NEWS_PANEL_WIDTH        576   // 右側 45%

// LLM Module UART Connection (M5-BUS: TX=37, RX=38)
#define LLM_UART_NUM            1
#define LLM_UART_TX_PIN         37    // Tab5 M5-BUS TX (GPIO 37)
#define LLM_UART_RX_PIN         38    // Tab5 M5-BUS RX (GPIO 38)
#define LLM_UART_BAUDRATE       115200

// I2C Internal Bus (RX8130CE RTC, GT911 Touch, INA226)
#define I2C_SDA_PIN             7
#define I2C_SCL_PIN             8
#define I2C_FREQ_HZ             400000

// ==========================================
// 3. News & RSS Feed Settings
// ==========================================
#define RSS_FETCH_INTERVAL_MS   (60 * 60 * 1000) // 1時間に1回自動更新 (バッテリー節電)
#define NEWS_AUTO_SLIDE_SEC     10               // 10秒ごとに記事を自動スライド
#define MAX_NEWS_ITEMS_PER_CAT  10
#define MAX_NEWS_CATEGORIES     4

// Yahoo! JAPAN ニュース RSS URLs
#define RSS_URL_TOP             "https://news.yahoo.co.jp/rss/topics/top-picks.xml"
#define RSS_URL_IT              "https://news.yahoo.co.jp/rss/topics/it.xml"
#define RSS_URL_BUSINESS        "https://news.yahoo.co.jp/rss/topics/business.xml"
#define RSS_URL_WORLD           "https://news.yahoo.co.jp/rss/topics/world.xml"

// ==========================================
// 4. FreeRTOS Task Priorities & Stack Sizes
// ==========================================
#define TASK_GUI_STACK_SIZE     (24 * 1024)
#define TASK_GUI_PRIORITY       5
#define TASK_GUI_CORE           1

#define TASK_NET_STACK_SIZE     (24 * 1024)
#define TASK_NET_PRIORITY       1
#define TASK_NET_CORE           0

#define TASK_LLM_STACK_SIZE     (16 * 1024)
#define TASK_LLM_PRIORITY       4
#define TASK_LLM_CORE           0
