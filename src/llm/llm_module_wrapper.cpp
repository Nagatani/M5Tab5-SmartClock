#include "llm_module_wrapper.h"
#include "intent_dispatcher.h"
#include <ArduinoJson.h>

#define TTS_QUEUE_MAX_ITEMS 8

LLMModuleWrapper::LLMModuleWrapper() {
    _ttsQueue = xQueueCreate(TTS_QUEUE_MAX_ITEMS, sizeof(TTSCommand*));
}

bool LLMModuleWrapper::init() {
    Serial.println("[LLM] Initializing M5Stack LLM Module via UART...");

    // Tab5 -> LLM Module シリアル初期化
    static HardwareSerial llmSerial(LLM_UART_NUM);
    _serial = &llmSerial;
    _serial->begin(LLM_UART_BAUDRATE, SERIAL_8N1, LLM_UART_RX_PIN, LLM_UART_TX_PIN);

    _module.begin(_serial);

    // モジュール接続確認 (Ping)
    int retries = 5;
    while (retries-- > 0) {
        if (_module.sys.ping() == 0) {
            Serial.println("[LLM] LLM Module Ping OK!");
            _isReady = true;
            break;
        }
        delay(500);
    }

    if (!_isReady) {
        Serial.println("[LLM] LLM Module not responding. Will retry in background task.");
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

    // 3. TTS (音声合成 - 日本語) 初期化
    m5_module_llm::ApiTtsSetupConfig_t ttsConfig;
    _ttsWorkId = _module.tts.setup(ttsConfig, "tts_setup", "ja_JP");
    Serial.printf("[LLM] TTS Setup ID: %s\n", _ttsWorkId.c_str());

    return true;
}

bool LLMModuleWrapper::enqueueTTS(const String& text, bool highPriority) {
    if (!_ttsQueue || text.length() == 0) return false;

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
        Serial.printf("[LLM] Playing TTS: %s\n", cmd->text.c_str());
        
        // M5ModuleLLM TTS 音声合成リクエスト
        _module.tts.inference(_ttsWorkId, cmd->text);

        delete cmd;
    }
}

void LLMModuleWrapper::update() {
    if (!_isReady) {
        // 未初期化の場合はリトライ
        if (_serial && _module.sys.ping() == 0) {
            init();
        }
        return;
    }

    // UART からのイベント受信とパース
    _module.update();

    // ASR / KWS 受信メッセージの処理
    for (auto& msg : _module.msg.responseMsgList) {
        if (_asrWorkId.length() > 0 && msg.work_id == _asrWorkId) {
            JsonDocument doc;
            DeserializationError err = deserializeJson(doc, msg.raw_msg);
            if (err == DeserializationError::Ok) {
                // ASR 認識テキストの抽出 (delta または text)
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

    // 処理済みメッセージリストのクリア
    _module.msg.responseMsgList.clear();

    // TTS キューの処理
    processTTSQueue();
}

void LLMModuleWrapper::updateStatus(SystemStatus& status) {
    status.llmModuleReady = _isReady;
}
