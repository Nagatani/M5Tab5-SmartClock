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

String RSSParser::cleanXmlText(const String& rawText) {
    String s = rawText;
    
    // CDATA セクションの除去: <![CDATA[テキスト]]>
    int cdataStart = s.indexOf("<![CDATA[");
    if (cdataStart >= 0) {
        int cdataEnd = s.indexOf("]]>", cdataStart + 9);
        if (cdataEnd > cdataStart) {
            s = s.substring(cdataStart + 9, cdataEnd);
        }
    }

    // HTML タグの簡易除去 (<p>, <br>, <a> など)
    while (true) {
        int tagOpen = s.indexOf('<');
        if (tagOpen < 0) break;
        int tagClose = s.indexOf('>', tagOpen);
        if (tagClose < 0) break;
        s.remove(tagOpen, tagClose - tagOpen + 1);
    }

    // HTML 特殊文字のデコード
    s.replace("&amp;", "&");
    s.replace("&quot;", "\"");
    s.replace("&#39;", "'");
    s.replace("&apos;", "'");
    s.replace("&lt;", "<");
    s.replace("&gt;", ">");
    s.replace("&nbsp;", " ");
    s.replace("&#8217;", "'");
    s.replace("&#8216;", "'");
    s.replace("&#8220;", "\"");
    s.replace("&#8221;", "\"");

    s.trim();
    return s;
}

String RSSParser::extractTagContent(const String& src, const String& tag, int startIdx, int endIdx) {
    String openTag = "<" + tag;
    String closeTag = "</" + tag + ">";

    int tagOpenPos = src.indexOf(openTag, startIdx);
    if (tagOpenPos < 0 || (endIdx > 0 && tagOpenPos >= endIdx)) return "";

    int contentStart = src.indexOf('>', tagOpenPos);
    if (contentStart < 0 || (endIdx > 0 && contentStart >= endIdx)) return "";
    contentStart += 1;

    int tagClosePos = src.indexOf(closeTag, contentStart);
    if (tagClosePos < 0 || (endIdx > 0 && tagClosePos > endIdx)) return "";

    return src.substring(contentStart, tagClosePos);
}

bool RSSParser::fetchCategory(NewsCategoryType catType, NewsCategoryData& outData) {
    const char* url = getCategoryUrl(catType);
    outData.name = getCategoryName(catType);
    outData.rssUrl = url;
    outData.itemCount = 0;

    Serial.printf("[RSS] Fetching %s RSS: %s\n", outData.name.c_str(), url);

    WiFiClientSecure client;
    client.setInsecure(); // SSL証明書検証スキップによる高速化

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

    // <item> タグごとのパース処理
    size_t count = 0;
    int curPos = 0;

    while (count < MAX_NEWS_ITEMS_PER_CAT) {
        int itemStart = payload.indexOf("<item>", curPos);
        if (itemStart < 0) {
            itemStart = payload.indexOf("<item ", curPos);
            if (itemStart < 0) break;
        }

        int itemEnd = payload.indexOf("</item>", itemStart);
        if (itemEnd < 0) break;

        String itemXml = payload.substring(itemStart, itemEnd + 7);

        String rawTitle = extractTagContent(itemXml, "title", 0, itemXml.length());
        String rawDesc  = extractTagContent(itemXml, "description", 0, itemXml.length());
        String rawPub   = extractTagContent(itemXml, "pubDate", 0, itemXml.length());
        String rawLink  = extractTagContent(itemXml, "link", 0, itemXml.length());

        NewsItem& item = outData.items[count];
        item.title = cleanXmlText(rawTitle);
        item.description = cleanXmlText(rawDesc);
        item.pubDate = cleanXmlText(rawPub);
        item.link = cleanXmlText(rawLink);

        // 日時フォーマットの簡易調整 (例: "16:30")
        int timeIdx = item.pubDate.indexOf(':');
        if (timeIdx >= 2) {
            item.pubDate = item.pubDate.substring(timeIdx - 2, timeIdx + 3);
        }

        if (item.title.length() > 0) {
            count++;
        }

        curPos = itemEnd + 7;
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
        delay(200);
    }
    return allSuccess;
}
