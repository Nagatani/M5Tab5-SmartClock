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

    // 0. モジュール内部の古いタスクを全リセット
    Serial.println("[LLM] Resetting existing module tasks...");
    _module.sys.reset();
    delay(600);

    // 1. Audio (サウンドカード・スピーカー出力) の初期化
    m5_module_llm::ApiAudioSetupConfig_t audioConfig;
    audioConfig.playVolume = 1.0;
    audioConfig.capVolume  = 1.0;
    String audioId = _module.audio.setup(audioConfig, "audio_setup");
    Serial.printf("[LLM] Audio Setup ID: %s\n", audioId.c_str());

    // 2. KWS (ウェイクワード) 初期化
    m5_module_llm::ApiKwsSetupConfig_t kwsConfig;
    kwsConfig.kws = "hi m5";
    _kwsWorkId = _module.kws.setup(kwsConfig, "kws_setup");
    Serial.printf("[LLM] KWS Setup ID: %s\n", _kwsWorkId.c_str());

    // 3. ASR (音声認識 - 日本語) 初期化
    m5_module_llm::ApiAsrSetupConfig_t asrConfig;
    asrConfig.input = {"sys.pcm", _kwsWorkId};
    _asrWorkId = _module.asr.setup(asrConfig, "asr_setup", "ja_JP");
    Serial.printf("[LLM] ASR Setup ID: %s\n", _asrWorkId.c_str());

    // 4. 日本語 MeloTTS のセットアップ (ライブラリの setup メソッドから動的 WorkID を取得)
    m5_module_llm::ApiMelottsSetupConfig_t meloConfig;
    meloConfig.model = "melotts-ja-jp";
    meloConfig.response_format = "sys.pcm";
    meloConfig.input = {"tts.utf-8.stream"};
    meloConfig.enaudio = true;
    meloConfig.enoutput = false;
    _ttsWorkId = _module.melotts.setup(meloConfig, "melo_setup", "ja_JP");

    if (_ttsWorkId.length() == 0) {
        _ttsWorkId = "melotts.1003";
    }

    Serial.printf("[LLM] MeloTTS (ja_JP) Active Work ID: %s\n", _ttsWorkId.c_str());
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
    if (!_isReady || !_ttsQueue || !_serial || _ttsWorkId.length() == 0) return;

    TTSCommand* cmd = nullptr;
    if (xQueueReceive(_ttsQueue, &cmd, 0) == pdTRUE && cmd != nullptr) {
        Serial.printf("[LLM] Executing Native MeloTTS Push: %s (Target WorkID: %s)\n", 
                      cmd->text.c_str(), _ttsWorkId.c_str());
        
        // StackFlow の動的 work_id 宛に Push
        JsonDocument doc;
        doc["request_id"]      = "push_" + String(millis());
        doc["work_id"]         = _ttsWorkId; // 例: "melotts.1003"
        doc["action"]          = "push";
        doc["object"]          = "tts.utf-8.stream";
        doc["data"]["delta"]   = cmd->text;
        doc["data"]["index"]   = 0;
        doc["data"]["finish"]  = true;

        String jsonStr;
        serializeJson(doc, jsonStr);
        jsonStr += "\n";

        _serial->print(jsonStr);
        Serial.printf("[LLM] Sent Stream Push JSON: %s", jsonStr.c_str());

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

    // モジュールからの全受信メッセージを処理
    for (auto& msg : _module.msg.responseMsgList) {
        Serial.printf("[LLM RAW RESPONSE] WorkID: %s, Msg: %s\n", msg.work_id.c_str(), msg.raw_msg.c_str());

        // MeloTTS の動的 WorkID の自動学習更新
        if (msg.work_id.startsWith("melotts.") && _ttsWorkId != msg.work_id) {
            _ttsWorkId = msg.work_id;
            Serial.printf("[LLM] Updated Active MeloTTS WorkID to: %s\n", _ttsWorkId.c_str());
        }

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

    // TTS キューの処理
    processTTSQueue();
}

void LLMModuleWrapper::updateStatus(SystemStatus& status) {
    status.llmModuleReady = _isReady;
}
