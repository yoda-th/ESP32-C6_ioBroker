// Firmware V0.9.0 – based on V0.8.0, modified in this version.
#include "wifi_module.h"
#include "config.h"
#include "logger.h"
#include "wifi_config_module.h"
#include "valve_module.h"

#include <WiFi.h>

static NetConfig netCfg;
static unsigned long lastReconnectAttempt = 0;
static unsigned long lastWifiOkMs = 0;
static const unsigned long WIFI_RECONNECT_INTERVAL_MS = 5000;
static const unsigned long WIFI_MAX_DOWN_MS = 5UL * 60UL * 1000UL;

static unsigned long reconnectCounter = 0; // V0.9 CHANGE: Reconnect-Zähler

void wifiInit() {
    pinMode(PIN_CFG_BUTTON, INPUT_PULLUP);
    if (digitalRead(PIN_CFG_BUTTON) == LOW) {
        wifiConfigPortal();
    }

    netcfgLoad(netCfg);
    logInfo("WiFi using SSID=" + netCfg.ssid + " MQTT Host=" + netCfg.mqttHost);

    WiFi.mode(WIFI_STA);
    WiFi.begin(netCfg.ssid.c_str(), netCfg.pass.c_str());
    lastReconnectAttempt = millis();
    lastWifiOkMs = millis();
    reconnectCounter = 0;
}

void wifiLoop() {
    if (WiFi.status() == WL_CONNECTED) {
        lastWifiOkMs = millis();
        reconnectCounter = 0; // V0.9 CHANGE
        return;
    }

    unsigned long now = millis();
    if (now - lastReconnectAttempt > WIFI_RECONNECT_INTERVAL_MS) {
        lastReconnectAttempt = now;
        reconnectCounter++;

        // V0.9 CHANGE (D1/D2): klarere Reconnect-Logs
        logWarn("WiFi disconnected, reconnect attempt #" + String(reconnectCounter));
        WiFi.disconnect(false, false);
        WiFi.mode(WIFI_STA);
        WiFi.begin(netCfg.ssid.c_str(), netCfg.pass.c_str());
    }

    if (now - lastWifiOkMs > WIFI_MAX_DOWN_MS) {
        logError("WiFi down > 5min, rebooting (safe valve close)");
        valveSafeBeforeUpdate();
        delay(500);
        ESP.restart();
    }
}

bool wifiIsConnected() {
    return WiFi.status() == WL_CONNECTED;
}

String wifiGetIp() {
    if (!wifiIsConnected()) return String("0.0.0.0");
    return WiFi.localIP().toString();
}

int wifiGetRssi() {
    if (!wifiIsConnected()) return 0;
    return WiFi.RSSI();
}

String wifiGetMqttHost() {
    return netCfg.mqttHost;
}
