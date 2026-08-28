#include "ui_manager.h"
#include "ui_theme.h"
#include "hal/bsp_tab5.h"
#include <esp_heap_caps.h>

#define LVGL_BUFFER_LINES 60
static lv_disp_draw_buf_t draw_buf;
static lv_color_t* buf1 = nullptr;
static lv_color_t* buf2 = nullptr;

UIManager::UIManager() {
    _guiMutex = xSemaphoreCreateMutex();
}

void UIManager::dispFlushCb(lv_disp_drv_t* disp, const lv_area_t* area, lv_color_t* color_p) {
    uint32_t w = (area->x2 - area->x1 + 1);
    uint32_t h = (area->y2 - area->y1 + 1);

    auto& display = BSPTab5::getInstance().getDisplay();
    display.startWrite();
    display.setAddrWindow(area->x1, area->y1, w, h);
    display.writePixels((uint16_t*)&color_p->full, w * h, true);
    display.endWrite();

    lv_disp_flush_ready(disp);
}

void UIManager::touchpadReadCb(lv_indev_drv_t* indev_drv, lv_indev_data_t* data) {
    lgfx::touch_point_t tp;
    auto& display = BSPTab5::getInstance().getDisplay();
    uint8_t count = display.getTouch(&tp, 1);

    if (count > 0) {
        data->state = LV_INDEV_STATE_PR;
        data->point.x = tp.x;
        data->point.y = tp.y;
    } else {
        data->state = LV_INDEV_STATE_REL;
    }
}

bool UIManager::init() {
    if (_initialized) return true;

    lv_init();

    // PSRAM 上にダブルバッファを割り当て (1280 x 60 lines)
    size_t buf_size = SCREEN_WIDTH * LVGL_BUFFER_LINES * sizeof(lv_color_t);
    buf1 = (lv_color_t*)heap_caps_malloc(buf_size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    buf2 = (lv_color_t*)heap_caps_malloc(buf_size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);

    if (!buf1) {
        buf1 = (lv_color_t*)malloc(buf_size);
    }
    if (!buf2) {
        buf2 = (lv_color_t*)malloc(buf_size);
    }

    lv_disp_draw_buf_init(&draw_buf, buf1, buf2, SCREEN_WIDTH * LVGL_BUFFER_LINES);

    // ディスプレイドライバ登録
    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = SCREEN_WIDTH;
    disp_drv.ver_res = SCREEN_HEIGHT;
    disp_drv.flush_cb = dispFlushCb;
    disp_drv.draw_buf = &draw_buf;
    lv_disp_drv_register(&disp_drv);

    // タッチドライバ登録
    static lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = touchpadReadCb;
    lv_indev_drv_register(&indev_drv);

    // テーマスタイル初期化
    UITheme::initStyles();

    // 画面背景設定
    lv_obj_t* scr = lv_scr_act();
    lv_obj_set_style_bg_color(scr, COLOR_BG_MAIN, 0);

    // 画面の左右パネルを初期化
    UIClock::getInstance().init(scr);
    UINews::getInstance().init(scr);

    _initialized = true;
    Serial.println("[UI] LVGL and UI Layout initialized successfully.");
    return true;
}

bool UIManager::lock(TickType_t timeout) {
    if (!_guiMutex) return false;
    return (xSemaphoreTake(_guiMutex, timeout) == pdTRUE);
}

void UIManager::unlock() {
    if (_guiMutex) {
        xSemaphoreGive(_guiMutex);
    }
}

void UIManager::loop() {
    if (lock(pdMS_TO_TICKS(50))) {
        lv_timer_handler();
        unlock();
    }
}
