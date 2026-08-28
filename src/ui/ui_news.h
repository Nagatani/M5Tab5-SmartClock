#pragma once

#include <Arduino.h>
#include <lvgl.h>
#include "types.h"
#include "config.h"

class UINews {
public:
    static UINews& getInstance() {
        static UINews instance;
        return instance;
    }

    void init(lv_obj_t* parent);
    void updateCategoryData(NewsCategoryType catType, const NewsCategoryData& data);
    void showNewsItem(size_t index);
    void nextNews();
    void prevNews();
    void selectCategory(NewsCategoryType catType);

    NewsCategoryType getCurrentCategory() const { return _currentCategory; }
    size_t getCurrentIndex() const { return _currentIndex; }
    const NewsItem* getCurrentNewsItem() const;

    // コールバック設定
    void setOnCategoryChangeCallback(void (*callback)(NewsCategoryType)) {
        _onCategoryChangeCallback = callback;
    }
    void setOnSpeakNewsCallback(void (*callback)(const NewsItem&)) {
        _onSpeakNewsCallback = callback;
    }

    void handleCategoryBtn(NewsCategoryType type);
    void handleSpeakCurrentNews();

private:
    UINews() = default;
    ~UINews() = default;

    lv_obj_t* _panel = nullptr;
    
    // Category Tabs
    lv_obj_t* _btnTabs[CAT_MAX] = {nullptr};
    NewsCategoryType _currentCategory = CAT_TOP;

    // News Card Components
    lv_obj_t* _cardPanel = nullptr;
    lv_obj_t* _lblNewsTitle = nullptr;
    lv_obj_t* _lblNewsDesc = nullptr;
    lv_obj_t* _lblNewsMeta = nullptr;
    lv_obj_t* _lblPageIndicator = nullptr;
    lv_obj_t* _barSlideProgress = nullptr;

    // Navigation & Action Buttons
    lv_obj_t* _btnPrev = nullptr;
    lv_obj_t* _btnNext = nullptr;
    lv_obj_t* _btnSpeakNews = nullptr;

    NewsCategoryData _categories[CAT_MAX];
    size_t _currentIndex = 0;

    void (*_onCategoryChangeCallback)(NewsCategoryType) = nullptr;
    void (*_onSpeakNewsCallback)(const NewsItem&) = nullptr;

    void refreshCardView();
};
