#pragma once

#include <Arduino.h>
#include <WiFi.h>
#include "config.h"
#include "types.h"

class WiFiManager {
public:
    static WiFiManager& getInstance() {
        static WiFiManager instance;
        return instance;
    }

    bool connect(const char* ssid = WIFI_SSID, const char* password = WIFI_PASSWORD);
    bool isConnected();
    void updateStatus(SystemStatus& status);
    bool checkAndReconnect();

private:
    WiFiManager() = default;
    ~WiFiManager() = default;

    unsigned long _lastReconnectAttempt = 0;
    bool _connecting = false;
};
