#include "ui_news.h"
#include "ui_theme.h"
#include "config.h"

static const char* CATEGORY_LABELS[CAT_MAX] = {
    "主要", "IT・科学", "経済", "国際"
};

static void tab_btn_event_cb(lv_event_t* e) {
    if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
        intptr_t catIndex = (intptr_t)lv_event_get_user_data(e);
        UINews::getInstance().handleCategoryBtn((NewsCategoryType)catIndex);
    }
}

static void btn_nav_event_cb(lv_event_t* e) {
    if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
        intptr_t dir = (intptr_t)lv_event_get_user_data(e);
        if (dir == 1) {
            UINews::getInstance().nextNews();
        } else if (dir == -1) {
            UINews::getInstance().prevNews();
        }
    }
}

static void btn_speak_news_event_cb(lv_event_t* e) {
    if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
        UINews::getInstance().handleSpeakCurrentNews();
    }
}

void UINews::init(lv_obj_t* parent) {
    // ====================================================================
    // 下部左側：Yahoo! ニュース パネル (625 x 290 px)
    // ====================================================================
    _panel = lv_obj_create(parent);
    lv_obj_set_pos(_panel, 10, CLOCK_PANEL_HEIGHT + 15);
    lv_obj_set_size(_panel, BOTTOM_PANEL_WIDTH, BOTTOM_PANEL_HEIGHT - 5);
    lv_obj_add_style(_panel, &UITheme::style_panel, 0);
    lv_obj_clear_flag(_panel, LV_OBJ_FLAG_SCROLLABLE);

    // --- 1. カテゴリ切替タブバー (上部) ---
    lv_obj_t* tabContainer = lv_obj_create(_panel);
    lv_obj_set_size(tabContainer, lv_pct(100), 38);
    lv_obj_align(tabContainer, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_style_bg_opa(tabContainer, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(tabContainer, 0, 0);
    lv_obj_set_style_pad_all(tabContainer, 0, 0);
    lv_obj_set_flex_flow(tabContainer, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(tabContainer, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    for (int i = 0; i < CAT_MAX; ++i) {
        _btnTabs[i] = lv_btn_create(tabContainer);
        lv_obj_set_size(_btnTabs[i], 140, 36);
        lv_obj_add_style(_btnTabs[i], &UITheme::style_tab_btn, 0);
        lv_obj_add_event_cb(_btnTabs[i], tab_btn_event_cb, LV_EVENT_CLICKED, (void*)(intptr_t)i);

        lv_obj_t* lbl = lv_label_create(_btnTabs[i]);
        lv_label_set_text(lbl, CATEGORY_LABELS[i]);
        lv_obj_set_style_text_font(lbl, UITheme::font_body_18, 0);
        lv_obj_center(lbl);
    }

    // --- 2. ニュースカードビュー (中央: 高さ 135px) ---
    _cardPanel = lv_obj_create(_panel);
    lv_obj_set_size(_cardPanel, lv_pct(100), 135);
    lv_obj_align(_cardPanel, LV_ALIGN_TOP_MID, 0, 42);
    lv_obj_set_style_bg_color(_cardPanel, lv_color_hex(0x181B22), 0);
    lv_obj_set_style_border_color(_cardPanel, COLOR_PANEL_BORDER, 0);
    lv_obj_set_style_radius(_cardPanel, 10, 0);
    lv_obj_set_style_pad_all(_cardPanel, 10, 0);

    // ニュースタイトル (日本語 TTF 18px〜24px)
    _lblNewsTitle = lv_label_create(_cardPanel);
    lv_label_set_text(_lblNewsTitle, "ニュースを取得中...");
    lv_obj_set_style_text_color(_lblNewsTitle, COLOR_TEXT_PRIMARY, 0);
    lv_obj_set_style_text_font(_lblNewsTitle, UITheme::font_title_24, 0);
    lv_label_set_long_mode(_lblNewsTitle, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(_lblNewsTitle, lv_pct(100));
    lv_obj_align(_lblNewsTitle, LV_ALIGN_TOP_LEFT, 0, 0);

    // ニュース概要
    _lblNewsDesc = lv_label_create(_cardPanel);
    lv_label_set_text(_lblNewsDesc, "");
    lv_obj_set_style_text_color(_lblNewsDesc, COLOR_TEXT_SECONDARY, 0);
    lv_obj_set_style_text_font(_lblNewsDesc, UITheme::font_small_14, 0);
    lv_label_set_long_mode(_lblNewsDesc, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(_lblNewsDesc, lv_pct(100));
    lv_obj_align(_lblNewsDesc, LV_ALIGN_TOP_LEFT, 0, 58);

    // メタ情報 (更新時刻)
    _lblNewsMeta = lv_label_create(_cardPanel);
    lv_obj_set_style_text_font(_lblNewsMeta, UITheme::font_small_14, 0);
    lv_label_set_text(_lblNewsMeta, "ソース: Yahoo!ニュース");
    lv_obj_set_style_text_color(_lblNewsMeta, COLOR_TEXT_MUTED, 0);
    lv_obj_align(_lblNewsMeta, LV_ALIGN_BOTTOM_LEFT, 0, 0);

    // ページインジケータ (1/8)
    _lblPageIndicator = lv_label_create(_cardPanel);
    lv_obj_set_style_text_font(_lblPageIndicator, UITheme::font_small_14, 0);
    lv_label_set_text(_lblPageIndicator, "0 / 0");
    lv_obj_set_style_text_color(_lblPageIndicator, COLOR_ACCENT_CYAN, 0);
    lv_obj_align(_lblPageIndicator, LV_ALIGN_BOTTOM_RIGHT, 0, 0);

    // 自動スライド進行度バー
    _barSlideProgress = lv_bar_create(_panel);
    lv_obj_set_size(_barSlideProgress, lv_pct(100), 3);
    lv_obj_align_to(_barSlideProgress, _cardPanel, LV_ALIGN_OUT_BOTTOM_MID, 0, 4);
    lv_bar_set_range(_barSlideProgress, 0, 100);
    lv_bar_set_value(_barSlideProgress, 0, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(_barSlideProgress, COLOR_ACCENT_BLUE, LV_PART_INDICATOR);

    // --- 3. 操作エリア (下部) ---
    lv_obj_t* bottomNav = lv_obj_create(_panel);
    lv_obj_set_size(bottomNav, lv_pct(100), 48);
    lv_obj_align(bottomNav, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_set_style_bg_opa(bottomNav, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(bottomNav, 0, 0);
    lv_obj_set_style_pad_all(bottomNav, 0, 0);
    lv_obj_set_flex_flow(bottomNav, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(bottomNav, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    // [前へ] ボタン
    _btnPrev = lv_btn_create(bottomNav);
    lv_obj_set_size(_btnPrev, 100, 46);
    lv_obj_add_style(_btnPrev, &UITheme::style_tab_btn, 0);
    lv_obj_add_event_cb(_btnPrev, btn_nav_event_cb, LV_EVENT_CLICKED, (void*)(intptr_t)-1);
    lv_obj_t* lblPrev = lv_label_create(_btnPrev);
    lv_obj_set_style_text_font(lblPrev, UITheme::font_body_18, 0);
    lv_label_set_text(lblPrev, LV_SYMBOL_LEFT " 前へ");
    lv_obj_center(lblPrev);

    // [ニュース読み上げ] ボタン
    _btnSpeakNews = lv_btn_create(bottomNav);
    lv_obj_set_size(_btnSpeakNews, 360, 46);
    lv_obj_add_style(_btnSpeakNews, &UITheme::style_btn_primary, 0);
    lv_obj_add_event_cb(_btnSpeakNews, btn_speak_news_event_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t* lblSpeak = lv_label_create(_btnSpeakNews);
    lv_obj_set_style_text_font(lblSpeak, UITheme::font_body_18, 0);
    lv_label_set_text(lblSpeak, LV_SYMBOL_VOLUME_MAX " ニュースを読み上げ");
    lv_obj_center(lblSpeak);

    // [次へ] ボタン
    _btnNext = lv_btn_create(bottomNav);
    lv_obj_set_size(_btnNext, 100, 46);
    lv_obj_add_style(_btnNext, &UITheme::style_tab_btn, 0);
    lv_obj_add_event_cb(_btnNext, btn_nav_event_cb, LV_EVENT_CLICKED, (void*)(intptr_t)1);
    lv_obj_t* lblNext = lv_label_create(_btnNext);
    lv_obj_set_style_text_font(lblNext, UITheme::font_body_18, 0);
    lv_label_set_text(lblNext, "次へ " LV_SYMBOL_RIGHT);
    lv_obj_center(lblNext);

    selectCategory(CAT_TOP);
}

void UINews::updateCategoryData(NewsCategoryType catType, const NewsCategoryData& data) {
    if (catType >= CAT_MAX) return;
    _categories[catType] = data;

    if (_currentCategory == catType) {
        if (_currentIndex >= data.itemCount) {
            _currentIndex = 0;
        }
        refreshCardView();
    }
}

void UINews::selectCategory(NewsCategoryType catType) {
    if (catType >= CAT_MAX) return;
    _currentCategory = catType;
    _currentIndex = 0;

    // タブのアクティブ色ハイライト更新
    for (int i = 0; i < CAT_MAX; ++i) {
        if (i == _currentCategory) {
            lv_obj_set_style_bg_color(_btnTabs[i], COLOR_ACCENT_BLUE, 0);
        } else {
            lv_obj_set_style_bg_color(_btnTabs[i], lv_color_hex(0x252A36), 0);
        }
    }

    refreshCardView();

    if (_onCategoryChangeCallback) {
        _onCategoryChangeCallback(_currentCategory);
    }
}

void UINews::showNewsItem(size_t index) {
    const auto& cat = _categories[_currentCategory];
    if (cat.itemCount == 0) return;

    if (index >= cat.itemCount) {
        _currentIndex = 0;
    } else {
        _currentIndex = index;
    }
    refreshCardView();
}

void UINews::nextNews() {
    const auto& cat = _categories[_currentCategory];
    if (cat.itemCount == 0) return;
    _currentIndex = (_currentIndex + 1) % cat.itemCount;
    refreshCardView();
}

void UINews::prevNews() {
    const auto& cat = _categories[_currentCategory];
    if (cat.itemCount == 0) return;
    if (_currentIndex == 0) {
        _currentIndex = cat.itemCount - 1;
    } else {
        _currentIndex--;
    }
    refreshCardView();
}

const NewsItem* UINews::getCurrentNewsItem() const {
    const auto& cat = _categories[_currentCategory];
    if (cat.itemCount == 0 || _currentIndex >= cat.itemCount) {
        return nullptr;
    }
    return &_categories[_currentCategory].items[_currentIndex];
}

void UINews::refreshCardView() {
    const auto& cat = _categories[_currentCategory];

    if (!cat.isLoaded || cat.itemCount == 0) {
        lv_label_set_text(_lblNewsTitle, "ニュースを取得中...");
        lv_label_set_text(_lblNewsDesc, "");
        lv_label_set_text(_lblNewsMeta, "更新待ち");
        lv_label_set_text(_lblPageIndicator, "0 / 0");
        return;
    }

    const NewsItem& item = cat.items[_currentIndex];
    lv_label_set_text(_lblNewsTitle, item.title.c_str());
    lv_label_set_text(_lblNewsDesc, item.description.length() > 0 ? item.description.c_str() : "");
    
    char metaBuf[128];
    snprintf(metaBuf, sizeof(metaBuf), "更新: %s", item.pubDate.c_str());
    lv_label_set_text(_lblNewsMeta, metaBuf);

    char pageBuf[32];
    snprintf(pageBuf, sizeof(pageBuf), "%d / %d", (int)(_currentIndex + 1), (int)cat.itemCount);
    lv_label_set_text(_lblPageIndicator, pageBuf);
}

void UINews::handleCategoryBtn(NewsCategoryType type) {
    selectCategory(type);
}

void UINews::handleSpeakCurrentNews() {
    const NewsItem* item = getCurrentNewsItem();
    if (item && _onSpeakNewsCallback) {
        _onSpeakNewsCallback(*item);
    }
}
