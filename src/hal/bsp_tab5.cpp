#include "bsp_tab5.h"

bool BSPTab5::init() {
    if (_initialized) return true;

    auto cfg = M5.config();
    cfg.serial_baudrate = 115200;
    cfg.clear_display = true;
    cfg.output_power = true;

    M5.begin(cfg);

    // M5GFX ディスプレイとタッチの初期化確認
    M5.Display.init();
    M5.Display.setRotation(1); // 横向き Landscape (1280x720)
    M5.Display.setBrightness(180);

    // I2C 内部バスの初期化 (RTC / Touch / INA226)
    Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN, I2C_FREQ_HZ);

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
