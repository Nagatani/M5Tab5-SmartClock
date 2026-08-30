#include "bsp_tab5.h"

bool BSPTab5::init() {
    if (_initialized) return true;

    auto cfg = M5.config();
    cfg.serial_baudrate = 115200;
    cfg.clear_display = true;
    cfg.output_power = true; // 外部ポート (5V/3.3V) への給電を有効化

    M5.begin(cfg);

    // 外部拡張ポート・LLM Module への電源供給を確実に有効化
    M5.Power.setExtOutput(true);

    // 内蔵スピーカーの音量設定 (0〜255)
    M5.Speaker.setVolume(200);

    // M5GFX ディスプレイとタッチの初期化確認
    M5.Display.init();
    // 画面の向き: 横向き反転 (180度回転 -> 正しい正位置表示 1280x720)
    M5.Display.setRotation(3); 
    M5.Display.setBrightness(180);

    // 電源安定化待ち
    delay(500);

    _initialized = true;
    Serial.println("[BSP] M5Stack Tab5 Hardware Initialized successfully.");
    return true;
}

void BSPTab5::updatePowerStatus(SystemStatus& status) {
    if (!_initialized) return;

    M5.update();
    status.batteryPercentage = M5.Power.getBatteryLevel();
    status.isCharging = M5.Power.isCharging();
}

void BSPTab5::setBrightness(uint8_t brightness) {
    M5.Display.setBrightness(brightness);
}
