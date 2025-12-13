// Firmware V1.2.7 - WiFi Module (Settings Integration Fix)
#include "wifi_module.h"
#include "config.h"
#include "logger.h"
#include "wifi_config_module.h"
#include "valve_module.h"
#include "settings_module.h" // <--- WICHTIG

#include <WiFi.h>

static NetConfig netCfg;
static unsigned long lastReconnectAttempt = 0;
static unsigned long lastWifiOkMs = 0;
static const unsigned long WIFI_RECONNECT_INTERVAL_MS = 5000;
static const unsigned long WIFI_MAX_DOWN_MS = 5UL * 60UL * 1000UL; 

static unsigned long reconnectCounter = 0; 

void wifiInit() {
    pinMode(PIN_CFG_BUTTON, INPUT_PULLUP);
    if (digitalRead(PIN_CFG_BUTTON) == LOW) {
        wifiConfigPortal(); // Blockiert, bis Config fertig
    }

    netcfgLoad(netCfg);
    
    // WICHTIG: Wir holen den MQTT Host aus den zentralen Settings für das Log,
    // damit wir sehen, was wirklich genutzt wird.
    String realMqttHost = settingsGetMqttHost();
    logInfo("WiFi using SSID=" + netCfg.ssid + " MQTT Host=" + realMqttHost);

    WiFi.mode(WIFI_STA);
    WiFi.begin(netCfg.ssid.c_str(), netCfg.pass.c_str());
    lastReconnectAttempt = millis();
    lastWifiOkMs = millis();
    reconnectCounter = 0;
}

void wifiLoop() {
    if (WiFi.status() == WL_CONNECTED) {
        lastWifiOkMs = millis();
        reconnectCounter = 0; 
        return;
    }

    unsigned long now = millis();
    if (now - lastReconnectAttempt > WIFI_RECONNECT_INTERVAL_MS) {
        lastReconnectAttempt = now;
        reconnectCounter++;

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