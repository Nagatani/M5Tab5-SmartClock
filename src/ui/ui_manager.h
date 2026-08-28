#pragma once

#include <Arduino.h>
#include <lvgl.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include "types.h"
#include "ui_clock.h"
#include "ui_news.h"

class UIManager {
public:
    static UIManager& getInstance() {
        static UIManager instance;
        return instance;
    }

    bool init();
    void loop();

    // スレッドセーフなLVGL操作のためのMutexロック
    bool lock(TickType_t timeout = portMAX_DELAY);
    void unlock();

    UIClock& getClockUI() { return UIClock::getInstance(); }
    UINews& getNewsUI() { return UINews::getInstance(); }

private:
    UIManager();
    ~UIManager() = default;

    SemaphoreHandle_t _guiMutex = nullptr;
    bool _initialized = false;

    static void dispFlushCb(lv_disp_drv_t* disp, const lv_area_t* area, lv_color_t* color_p);
    static void touchpadReadCb(lv_indev_drv_t* indev_drv, lv_indev_data_t* data);
};
