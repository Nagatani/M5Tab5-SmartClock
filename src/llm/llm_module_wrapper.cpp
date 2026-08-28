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
    Serial.printf("[LLM] KWS Setup ID: %d\n", _kwsWorkId);

    // 2. ASR (音声認識 - 日本語) 初期化
    m5_module_llm::ApiAsrSetupConfig_t asrConfig;
    asrConfig.language = "ja_JP";
    _asrWorkId = _module.asr.setup(asrConfig, "asr_setup");
    Serial.printf("[LLM] ASR Setup ID: %d\n", _asrWorkId);

    // 3. TTS (音声合成 - 日本語) 初期化
    m5_module_llm::ApiTtsSetupConfig_t ttsConfig;
    _ttsWorkId = _module.tts.setup(ttsConfig, "tts_setup");
    Serial.printf("[LLM] TTS Setup ID: %d\n", _ttsWorkId);

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
    if (!_isReady || !_ttsQueue) return;

    TTSCommand* cmd = nullptr;
    if (xQueueReceive(_ttsQueue, &cmd, 0) == pdTRUE && cmd != nullptr) {
        Serial.printf("[LLM] Playing TTS: %s\n", cmd->text.c_str());
        
        // M5ModuleLLM TTS 音声合成リクエスト
        _module.tts.inference(_ttsWorkId, cmd->text.c_str());

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

    // ASR 音声認識結果のチェック
    if (_asrWorkId >= 0 && _module.asr.hasResult(_asrWorkId)) {
        auto result = _module.asr.getResult(_asrWorkId);
        if (result.is_final && strlen(result.text) > 0) {
            String recognized = String(result.text);
            Serial.printf("[LLM] ASR Recognized: %s\n", recognized.c_str());

            NewsCategoryType cat = CAT_TOP;
            VoiceIntentType intent = IntentDispatcher::getInstance().parseIntent(recognized, cat);

            VoiceEvent ev;
            ev.intent = intent;
            ev.recognizedText = recognized;
            ev.timestamp = millis();

            if (_onVoiceEventCallback) {
                _onVoiceEventCallback(ev);
            }
        }
    }

    // TTS キューの処理
    processTTSQueue();
}

void LLMModuleWrapper::updateStatus(SystemStatus& status) {
    status.llmModuleReady = _isReady;
}
