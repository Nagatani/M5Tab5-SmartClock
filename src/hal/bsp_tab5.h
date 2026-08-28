#pragma once

#include <Arduino.h>
#include <M5Unified.h>
#include "config.h"
#include "types.h"

class BSPTab5 {
public:
    static BSPTab5& getInstance() {
        static BSPTab5 instance;
        return instance;
    }

    bool init();
    void updatePowerStatus(SystemStatus& status);
    
    // M5GFX Display への参照
    M5GFX& getDisplay() { return M5.Display; }

    // バックライト輝度設定 (0 - 255)
    void setBrightness(uint8_t brightness);

private:
    BSPTab5() = default;
    ~BSPTab5() = default;
    BSPTab5(const BSPTab5&) = delete;
    BSPTab5& operator=(const BSPTab5&) = delete;

    bool _initialized = false;
};
