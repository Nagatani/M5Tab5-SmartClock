#pragma once

#include <Arduino.h>
#include "types.h"

class IntentDispatcher {
public:
    static IntentDispatcher& getInstance() {
        static IntentDispatcher instance;
        return instance;
    }

    // 音声認識テキストを解析してインテントを特定
    VoiceIntentType parseIntent(const String& recognizedText, NewsCategoryType& targetCat);

    // 意図に応じた定型応答テキストを生成
    String generateActionResponse(VoiceIntentType intent, const String& extraInfo = "");

private:
    IntentDispatcher() = default;
    ~IntentDispatcher() = default;
};
