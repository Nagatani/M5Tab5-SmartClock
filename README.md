# M5Stack Tab5 + LLM Module スマートクロック＆ニュース端末

M5Stack Tab5 (ESP32-P4 + ESP32-C6 / 1280x720 MIPI-DSI) と M5Stack LLM Module (AX630C NPU) を連携させた、スマートクロック＆カテゴリ別ニュースリーダー端末です。

---

## 1. 特徴・主要機能

1. **スマートクロック (左側 55% 画面エリア):**
   - 1280x720 の超大画面を活かしたデジタル時計 (`HH:MM:SS`)、日付、日本語曜日表示。
   - NTP (JST 日本標準時) 同期 ＋ 内蔵 RTC (RX8130CE) による正確なオフライン計時。
   - Wi-Fi 電波強度、RTC 状態、LLM 接続状態、バッテリー残量表示。
   - 音声認識字幕表示（マイク入力・アシスタント応答）。
   - 「現在時刻を音声で読み上げる」ワンタッチボタン。

2. **ジャンル別ニュースリーダー (右側 45% 画面エリア):**
   - Yahoo!ニュース RSS から「主要」「IT・科学」「経済」「国際」の最新記事を定期取得。
   - タイトル・要約本文・配信時刻をカード形式で表示。
   - 10秒ごとの自動スライド ＆ 手動送り（前へ/次へ）ナビゲーション。
   - 「このニュースを読み上げ」ボタンによる TTS 読み上げ。

3. **M5Stack LLM Module (AX630C) 連携:**
   - UART (115200bps / M5ModuleLLM) 通信。
   - KWS (ウェイクワード) ＆ ASR (音声認識) によるハンズフリー操作:
     - 「今の時間を教えて」「何時？」 -> 時刻を音声で返答
     - 「ニュースを読んで」 -> 表示中のニュースを読み上げ
     - 「次のニュース」「前のニュース」 -> 記事をスライド
     - 「ITニュース」「経済ニュース」 -> カテゴリを切り替え

4. **堅牢な FreeRTOS マルチタスク設計:**
   - `Task_GUI` (Core 1): LVGL 描画ループ (10ms) & タッチ入力。
   - `Task_Network` (Core 0): Wi-Fi 常時監視、NTP 同期、RSS XML 取得・パース。
   - `Task_LLM_Com` (Core 0): UART 受信、音声認識インテント解析、TTS キュー管理。
   - 全共有リソースを Mutex でスレッドセーフに保護。

---

## 2. ディレクトリ構成

```text
m5tab5-smart-clock/
├── platformio.ini               # PlatformIO ビルド設定 (ESP32-P4, M5Unified, LVGL, etc.)
├── include/
│   ├── config.h                 # Wi-Fi設定、RSS URL、ピン定義、タスク設定
│   ├── types.h                  # 共通データ構造体 (NewsItem, VoiceEvent, etc.)
│   └── lv_conf.h                # LVGL 8.4 設定 (RGB565, 128KB メモリ, UTF-8)
├── src/
│   ├── main.cpp                 # 全タスク初期化・FreeRTOS オーケストレーション
│   ├── hal/
│   │   ├── bsp_tab5.h / .cpp    # Tab5 ハードウェア (Display, Touch, Power)
│   │   └── rtc_manager.h / .cpp # RX8130CE RTC & NTP JST 同期
│   ├── ui/
│   │   ├── ui_manager.h / .cpp  # LVGL 描画バッファ (PSRAM) & 排他制御
│   │   ├── ui_theme.h / .cpp    # ダークモダンスタイル & パレット
│   │   ├── ui_clock.h / .cpp    # 左側 55% 時計・ステータス・字幕 UI
│   │   └── ui_news.h / .cpp     # 右側 45% ニュースカード・タブ・操作 UI
│   ├── network/
│   │   ├── wifi_manager.h / .cpp# Wi-Fi 接続監視・自動再接続
│   │   └── rss_parser.h / .cpp  # Yahoo! RSS (XML) HTTP取得 & TinyXML2 パース
│   └── llm/
│       ├── llm_module_wrapper.h / .cpp # M5ModuleLLM (UART) ラッパー (ASR/TTS/KWS)
│       └── intent_dispatcher.h / .cpp  # 音声コマンドインテント判定
└── README.md
```

---

## 3. 設定とビルド手順

### 1. Wi-Fi 設定の変更
`include/config.h` を開き、お使いの Wi-Fi 環境に合わせて SSID とパスワードを設定します。
```cpp
#define WIFI_SSID       "YOUR_WIFI_SSID"
#define WIFI_PASSWORD   "YOUR_WIFI_PASSWORD"
```

### 2. ハードウェア接続 (UART)
Tab5 と M5Stack LLM Module を接続します：
- **M5-BUS スタック接続** または **Grove ケーブル接続**:
  - `LLM_UART_TX_PIN`: GPIO 19 (Tab5 TX -> LLM RX)
  - `LLM_UART_RX_PIN`: GPIO 20 (Tab5 RX <- LLM TX)
  - （Grove ポート接続に変更する場合は `include/config.h` で GPIO 1 / GPIO 3 に変更可能）

### 3. ビルド ＆ アップロード (PlatformIO)
VS Code の PlatformIO 拡張機能、または CLI から実行します：

```bash
# プロジェクトディレクトリへ移動
cd C:\Users\nagat\.gemini\antigravity\scratch\m5tab5-smart-clock

# ビルド
pio run

# Tab5 への書き込み & シリアルモニタ起動
pio run -t upload -t monitor
```
