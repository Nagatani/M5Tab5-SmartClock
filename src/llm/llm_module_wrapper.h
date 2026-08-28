#pragma once

#include <Arduino.h>
#include <M5ModuleLLM.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include "config.h"
#include "types.h"

class LLMModuleWrapper {
public:
    static LLMModuleWrapper& getInstance() {
        static LLMModuleWrapper instance;
        return instance;
    }

    bool init();
    void update();

    // TTS 発話キューへの投入
    bool enqueueTTS(const String& text, bool highPriority = false);

    // 音声認識イベントリスナー登録
    void setOnVoiceEventCallback(void (*callback)(const VoiceEvent&)) {
        _onVoiceEventCallback = callback;
    }

    bool isReady() const { return _isReady; }
    void updateStatus(SystemStatus& status);

private:
    LLMModuleWrapper();
    ~LLMModuleWrapper() = default;

    M5ModuleLLM _module;
    HardwareSerial* _serial = nullptr;
    QueueHandle_t _ttsQueue = nullptr;

    bool _isReady = false;
    String _kwsWorkId = "";
    String _asrWorkId = "";
    String _ttsWorkId = "";
    String _llmWorkId = "";

    void (*_onVoiceEventCallback)(const VoiceEvent&) = nullptr;

    void processTTSQueue();
};
