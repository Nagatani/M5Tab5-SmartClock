#include "intent_dispatcher.h"

VoiceIntentType IntentDispatcher::parseIntent(const String& recognizedText, NewsCategoryType& targetCat) {
    String text = recognizedText;
    text.toLowerCase();

    // 1. 時刻の問い合わせ
    if (text.indexOf("時間") >= 0 || text.indexOf("何時") >= 0 || text.indexOf("いまなんじ") >= 0 || text.indexOf("time") >= 0) {
        return INTENT_TELL_TIME;
    }

    // 2. ニュース読み上げ
    if (text.indexOf("ニュース読") >= 0 || text.indexOf("読んで") >= 0 || text.indexOf("記事") >= 0 || text.indexOf("詳細") >= 0) {
        return INTENT_READ_NEWS;
    }

    // 3. 次へ
    if (text.indexOf("次") >= 0 || text.indexOf("つぎ") >= 0 || text.indexOf("next") >= 0) {
        return INTENT_NEXT_NEWS;
    }

    // 4. 前へ
    if (text.indexOf("前") >= 0 || text.indexOf("まえ") >= 0 || text.indexOf("もど") >= 0 || text.indexOf("prev") >= 0) {
        return INTENT_PREV_NEWS;
    }

    // 5. カテゴリ変更
    if (text.indexOf("it") >= 0 || text.indexOf("科学") >= 0 || text.indexOf("テクノロジー") >= 0) {
        targetCat = CAT_IT;
        return INTENT_CHANGE_CATEGORY;
    }
    if (text.indexOf("経済") >= 0 || text.indexOf("ビジネス") >= 0 || text.indexOf("株") >= 0) {
        targetCat = CAT_BUSINESS;
        return INTENT_CHANGE_CATEGORY;
    }
    if (text.indexOf("国際") >= 0 || text.indexOf("海外") >= 0 || text.indexOf("world") >= 0) {
        targetCat = CAT_WORLD;
        return INTENT_CHANGE_CATEGORY;
    }
    if (text.indexOf("主要") >= 0 || text.indexOf("トップ") >= 0) {
        targetCat = CAT_TOP;
        return INTENT_CHANGE_CATEGORY;
    }

    return INTENT_FREE_CHAT;
}

String IntentDispatcher::generateActionResponse(VoiceIntentType intent, const String& extraInfo) {
    switch (intent) {
        case INTENT_TELL_TIME:
            return extraInfo; // 時刻文字列
        case INTENT_READ_NEWS:
            return "ニュースを読み上げます。";
        case INTENT_NEXT_NEWS:
            return "次のニュースを表示します。";
        case INTENT_PREV_NEWS:
            return "前のニュースを表示します。";
        case INTENT_CHANGE_CATEGORY:
            return "カテゴリを切り替えました。";
        default:
            return extraInfo;
    }
}
