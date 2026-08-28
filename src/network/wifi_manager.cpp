#include "wifi_manager.h"

bool WiFiManager::connect(const char* ssid, const char* password) {
    if (WiFi.status() == WL_CONNECTED) return true;

    Serial.printf("[WiFi] Connecting to %s ...\n", ssid);
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);

    unsigned long startMs = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - startMs < WIFI_CONNECT_TIMEOUT_MS) {
        delay(500);
        Serial.print(".");
    }

    if (WiFi.status() == WL_CONNECTED) {
        Serial.printf("\n[WiFi] Connected! IP: %s (RSSI: %ddBm)\n", 
                      WiFi.localIP().toString().c_str(), WiFi.RSSI());
        return true;
    } else {
        Serial.println("\n[WiFi] Connection timeout.");
        return false;
    }
}

bool WiFiManager::isConnected() {
    return WiFi.status() == WL_CONNECTED;
}

void WiFiManager::updateStatus(SystemStatus& status) {
    status.wifiConnected = isConnected();
    if (status.wifiConnected) {
        status.wifiRssi = WiFi.RSSI();
        status.ipAddress = WiFi.localIP().toString();
    } else {
        status.wifiRssi = 0;
        status.ipAddress = "";
    }
}

bool WiFiManager::checkAndReconnect() {
    if (isConnected()) return true;

    if (millis() - _lastReconnectAttempt > 10000) {
        _lastReconnectAttempt = millis();
        Serial.println("[WiFi] Connection lost. Attempting to reconnect...");
        WiFi.disconnect();
        WiFi.reconnect();
    }
    return false;
}
