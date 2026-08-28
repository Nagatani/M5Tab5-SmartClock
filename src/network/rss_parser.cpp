#include "rss_parser.h"

const char* RSSParser::getCategoryUrl(NewsCategoryType type) {
    switch (type) {
        case CAT_TOP:      return RSS_URL_TOP;
        case CAT_IT:       return RSS_URL_IT;
        case CAT_BUSINESS: return RSS_URL_BUSINESS;
        case CAT_WORLD:    return RSS_URL_WORLD;
        default:           return RSS_URL_TOP;
    }
}

const char* RSSParser::getCategoryName(NewsCategoryType type) {
    switch (type) {
        case CAT_TOP:      return "主要";
        case CAT_IT:       return "IT・科学";
        case CAT_BUSINESS: return "経済";
        case CAT_WORLD:    return "国際";
        default:           return "主要";
    }
}

String RSSParser::cleanXmlText(const char* text) {
    if (!text) return "";
    String s = text;
    s.replace("&amp;", "&");
    s.replace("&quot;", "\"");
    s.replace("&#39;", "'");
    s.replace("&lt;", "<");
    s.replace("&gt;", ">");
    s.trim();
    return s;
}

bool RSSParser::fetchCategory(NewsCategoryType catType, NewsCategoryData& outData) {
    const char* url = getCategoryUrl(catType);
    outData.name = getCategoryName(catType);
    outData.rssUrl = url;
    outData.itemCount = 0;

    Serial.printf("[RSS] Fetching %s RSS: %s\n", outData.name.c_str(), url);

    WiFiClientSecure client;
    client.setInsecure(); // SSL証明書検証をスキップして軽量高速化

    HTTPClient http;
    http.setUserAgent("Mozilla/5.0 (M5StackTab5-SmartClock)");
    http.setTimeout(8000);

    if (!http.begin(client, url)) {
        Serial.printf("[RSS] HTTP begin failed for %s\n", url);
        return false;
    }

    int httpCode = http.GET();
    if (httpCode != HTTP_CODE_OK) {
        Serial.printf("[RSS] HTTP GET failed, error code: %d\n", httpCode);
        http.end();
        return false;
    }

    String payload = http.getString();
    http.end();

    if (payload.length() == 0) {
        Serial.println("[RSS] Empty response received.");
        return false;
    }

    // TinyXML2 による XML パース
    tinyxml2::XMLDocument doc;
    tinyxml2::XMLError err = doc.Parse(payload.c_str());
    if (err != tinyxml2::XML_SUCCESS) {
        Serial.printf("[RSS] XML Parse error: %d\n", err);
        return false;
    }

    tinyxml2::XMLElement* rss = doc.FirstChildElement("rss");
    if (!rss) return false;

    tinyxml2::XMLElement* channel = rss->FirstChildElement("channel");
    if (!channel) return false;

    tinyxml2::XMLElement* itemElem = channel->FirstChildElement("item");
    size_t count = 0;

    while (itemElem && count < MAX_NEWS_ITEMS_PER_CAT) {
        tinyxml2::XMLElement* titleElem = itemElem->FirstChildElement("title");
        tinyxml2::XMLElement* descElem  = itemElem->FirstChildElement("description");
        tinyxml2::XMLElement* pubElem   = itemElem->FirstChildElement("pubDate");
        tinyxml2::XMLElement* linkElem  = itemElem->FirstChildElement("link");

        NewsItem& item = outData.items[count];
        item.title = titleElem ? cleanXmlText(titleElem->GetText()) : "";
        item.description = descElem ? cleanXmlText(descElem->GetText()) : "";
        item.pubDate = pubElem ? cleanXmlText(pubElem->GetText()) : "";
        item.link = linkElem ? cleanXmlText(linkElem->GetText()) : "";

        // 日時フォーマットの簡易調整 (例: "Fri, 28 Aug 2026 16:30:00 +0900" -> "16:30")
        int timeIdx = item.pubDate.indexOf(':');
        if (timeIdx >= 2) {
            item.pubDate = item.pubDate.substring(timeIdx - 2, timeIdx + 3);
        }

        count++;
        itemElem = itemElem->NextSiblingElement("item");
    }

    outData.itemCount = count;
    outData.isLoaded = (count > 0);

    Serial.printf("[RSS] Successfully parsed %d items for category %s\n", count, outData.name.c_str());
    return true;
}

bool RSSParser::fetchAllCategories(NewsCategoryData categories[CAT_MAX]) {
    bool allSuccess = true;
    for (int i = 0; i < CAT_MAX; ++i) {
        bool success = fetchCategory((NewsCategoryType)i, categories[i]);
        if (!success) {
            allSuccess = false;
        }
        delay(200); // 連続リクエストの間隔
    }
    return allSuccess;
}
