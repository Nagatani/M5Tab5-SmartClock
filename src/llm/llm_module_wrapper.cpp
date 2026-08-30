#include "llm_module_wrapper.h"
#include "intent_dispatcher.h"
#include "hal/bsp_tab5.h"
#include <ArduinoJson.h>

#define TTS_QUEUE_MAX_ITEMS 4

struct UartPinCandidate {
    int rx;
    int tx;
    const char* label;
};

LLMModuleWrapper::LLMModuleWrapper() {
    _ttsQueue = xQueueCreate(TTS_QUEUE_MAX_ITEMS, sizeof(TTSCommand*));
}

bool LLMModuleWrapper::init() {
    Serial.println("[LLM] Initializing M5Stack LLM Module via UART...");

    int autoRx = M5.getPin(m5::pin_name_t::port_c_rxd);
    int autoTx = M5.getPin(m5::pin_name_t::port_c_txd);
    Serial.printf("[LLM] M5Unified Port.C Pin: RX=%d, TX=%d\n", autoRx, autoTx);

    static HardwareSerial llmSerial(LLM_UART_NUM);
    _serial = &llmSerial;

    UartPinCandidate candidates[] = {
        {autoRx, autoTx, "M5Unified Port.C Auto"},
        {38, 37, "Tab5 M5-BUS (RX:38, TX:37)"},
        {37, 38, "Tab5 M5-BUS Inverted (RX:37, TX:38)"},
        {20, 19, "Tab5 Port.C (RX:20, TX:19)"},
        {19, 20, "Tab5 Port.C Inverted (RX:19, TX:20)"},
        {3, 1, "Grove Port (RX:3, TX:1)"},
        {1, 3, "Grove Port Inverted (RX:1, TX:3)"}
    };
    size_t numCandidates = sizeof(candidates) / sizeof(candidates[0]);

    for (size_t i = 0; i < numCandidates; ++i) {
        if (candidates[i].rx <= 0 || candidates[i].tx <= 0) continue;

        Serial.printf("[LLM] Probing %s (RX:%d, TX:%d)...\n", 
                      candidates[i].label, candidates[i].rx, candidates[i].tx);
        
        _serial->begin(LLM_UART_BAUDRATE, SERIAL_8N1, candidates[i].rx, candidates[i].tx);
        delay(50);
        _module.begin(_serial);

        if (_module.checkConnection() || _module.sys.ping() == 0) {
            Serial.printf("[LLM] Connected successfully on %s!\n", candidates[i].label);
            _isReady = true;
            break;
        }
    }

    if (!_isReady) {
        Serial.println("[LLM] Connection check failed. Will retry in background task...");
        return false;
    }

    // 1. KWS (ウェイクワード) 初期化
    m5_module_llm::ApiKwsSetupConfig_t kwsConfig;
    kwsConfig.kws = "hi m5";
    _kwsWorkId = _module.kws.setup(kwsConfig, "kws_setup");
    Serial.printf("[LLM] KWS Setup ID: %s\n", _kwsWorkId.c_str());

    // 2. ASR (音声認識 - 日本語) 初期化
    m5_module_llm::ApiAsrSetupConfig_t asrConfig;
    asrConfig.input = {"sys.pcm", _kwsWorkId};
    _asrWorkId = _module.asr.setup(asrConfig, "asr_setup", "ja_JP");
    Serial.printf("[LLM] ASR Setup ID: %s\n", _asrWorkId.c_str());

    // 3. TTS (日本語 音声合成) 初期化
    // MeloTTS 日本語モデル (model-melotts-ja-jp / ja_JP) を優先設定
    m5_module_llm::ApiMelottsSetupConfig_t meloConfig;
    meloConfig.model = "model-melotts-ja-jp";
    _ttsWorkId = _module.melotts.setup(meloConfig, "tts_setup", "ja_JP");

    if (_ttsWorkId.length() == 0) {
        // MeloTTS で取得できない場合は標準 TTS (ja_JP) をフォールバック
        m5_module_llm::ApiTtsSetupConfig_t ttsConfig;
        ttsConfig.model = "model-melotts-ja-jp";
        _ttsWorkId = _module.tts.setup(ttsConfig, "tts_setup", "ja_JP");
    }

    Serial.printf("[LLM] TTS Japanese Setup ID: %s\n", _ttsWorkId.c_str());
    return true;
}

bool LLMModuleWrapper::enqueueTTS(const String& text, bool highPriority) {
    if (!_ttsQueue || text.length() == 0) return false;

    // 高優先度 (ボタンタップ時など) はキューの古い未再生テキストをフラッシュして即時反応
    if (highPriority) {
        TTSCommand* oldCmd = nullptr;
        while (xQueueReceive(_ttsQueue, &oldCmd, 0) == pdTRUE) {
            delete oldCmd;
        }
    }

    TTSCommand* cmd = new TTSCommand{text, highPriority};
    if (highPriority) {
        return (xQueueSendToFront(_ttsQueue, &cmd, 0) == pdTRUE);
    } else {
        return (xQueueSend(_ttsQueue, &cmd, 0) == pdTRUE);
    }
}

void LLMModuleWrapper::processTTSQueue() {
    if (!_isReady || !_ttsQueue || _ttsWorkId.length() == 0) return;

    TTSCommand* cmd = nullptr;
    if (xQueueReceive(_ttsQueue, &cmd, 0) == pdTRUE && cmd != nullptr) {
        Serial.printf("[LLM] Fast Playing TTS (JA): %s\n", cmd->text.c_str());
        
        // MeloTTS または TTS による日本語音声合成
        int ret = _module.melotts.inference(_ttsWorkId, cmd->text, 3000);
        if (ret != 0) {
            _module.tts.inference(_ttsWorkId, cmd->text, 3000);
        }

        delete cmd;
    }
}

void LLMModuleWrapper::update() {
    static unsigned long lastRetryMs = 0;
    static int retryCandidateIdx = 0;

    if (!_isReady) {
        if (millis() - lastRetryMs > 2000) {
            lastRetryMs = millis();
            
            UartPinCandidate candidates[] = {
                {38, 37, "Tab5 M5-BUS (RX:38, TX:37)"},
                {37, 38, "Tab5 M5-BUS Inverted (RX:37, TX:38)"},
                {20, 19, "Tab5 Port.C (RX:20, TX:19)"},
                {19, 20, "Tab5 Port.C Inverted (RX:19, TX:20)"},
                {3, 1, "Grove Port (RX:3, TX:1)"},
                {1, 3, "Grove Port Inverted (RX:1, TX:3)"}
            };
            size_t numCandidates = sizeof(candidates) / sizeof(candidates[0]);

            auto& cand = candidates[retryCandidateIdx];
            retryCandidateIdx = (retryCandidateIdx + 1) % numCandidates;

            _serial->begin(LLM_UART_BAUDRATE, SERIAL_8N1, cand.rx, cand.tx);
            delay(50);
            _module.begin(_serial);

            if (_module.checkConnection() || _module.sys.ping() == 0) {
                Serial.printf("[LLM] LLM Module detected on %s! Initializing...\n", cand.label);
                init();
            }
        }
        return;
    }

    // UART イベント処理
    _module.update();

    // ASR 受信
    for (auto& msg : _module.msg.responseMsgList) {
        if (_asrWorkId.length() > 0 && msg.work_id == _asrWorkId) {
            JsonDocument doc;
            DeserializationError err = deserializeJson(doc, msg.raw_msg);
            if (err == DeserializationError::Ok) {
                String recognizedText = "";
                if (doc["data"].is<JsonObject>()) {
                    if (doc["data"]["delta"].is<const char*>()) {
                        recognizedText = doc["data"]["delta"].as<String>();
                    } else if (doc["data"]["text"].is<const char*>()) {
                        recognizedText = doc["data"]["text"].as<String>();
                    }
                }

                recognizedText.trim();
                if (recognizedText.length() > 0) {
                    Serial.printf("[LLM] ASR Recognized: %s\n", recognizedText.c_str());

                    NewsCategoryType cat = CAT_TOP;
                    VoiceIntentType intent = IntentDispatcher::getInstance().parseIntent(recognizedText, cat);

                    VoiceEvent ev;
                    ev.intent = intent;
                    ev.recognizedText = recognizedText;
                    ev.timestamp = millis();

                    if (_onVoiceEventCallback) {
                        _onVoiceEventCallback(ev);
                    }
                }
            }
        }
    }

    _module.msg.responseMsgList.clear();

    // TTS キューの高速処理
    processTTSQueue();
}

void LLMModuleWrapper::updateStatus(SystemStatus& status) {
    status.llmModuleReady = _isReady;
}
