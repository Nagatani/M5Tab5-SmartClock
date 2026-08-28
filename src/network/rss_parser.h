#pragma once

#include <Arduino.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <tinyxml2.h>
#include "config.h"
#include "types.h"

class RSSParser {
public:
    static RSSParser& getInstance() {
        static RSSParser instance;
        return instance;
    }

    // 特定カテゴリのRSSを取得してパース
    bool fetchCategory(NewsCategoryType catType, NewsCategoryData& outData);

    // 全カテゴリを一括取得
    bool fetchAllCategories(NewsCategoryData categories[CAT_MAX]);

private:
    RSSParser() = default;
    ~RSSParser() = default;

    const char* getCategoryUrl(NewsCategoryType type);
    const char* getCategoryName(NewsCategoryType type);
    String cleanXmlText(const char* text);
};
