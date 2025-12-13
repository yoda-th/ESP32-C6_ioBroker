#include "mqtt_module.h"
#include "config.h"
#include "logger.h"
#include "wifi_module.h"
#include "settings_module.h" 
#include <WiFi.h>
#include <PubSubClient.h>
#include <vector> 

static WiFiClient espClient;
static PubSubClient mqttClient(espClient);
static MqttCommandCallback commandCallback = nullptr; 
static unsigned long lastMqttReconnectAttempt = 0;
static std::vector<String> historyQueue; 

// === GETTER FÜR DIAGNOSE ===
size_t mqttGetQueueSize() {
    return historyQueue.size();
}

unsigned long mqttGetLastReconnectMs() {
    // Alter des letzten Reconnect-Versuchs (ms) für Web-Diag
    if (lastMqttReconnectAttempt == 0) return 0;
    return millis() - lastMqttReconnectAttempt;
}

String mqttGetStateString() { return mqttClient.connected() ? "Connected" : "Disconnected"; }
String mqttGetLastError() { return String(mqttClient.state()); }
bool mqttIsConnected() { return mqttClient.connected(); }

static void mqttOnMessage(char* topic, byte* payload, unsigned int length) {
    String msg;
    for (unsigned int i=0; i<length; i++) msg += (char)payload[i];
    // Command Topic Check
    if (String(topic) == TOPIC_CMD && commandCallback) {
        commandCallback(msg);
    }
}

void mqttGracefulRestart() {
    if (mqttClient.connected()) {
        mqttClient.publish(TOPIC_LWT, MQTT_PAYLOAD_OFFLINE, true);
        mqttClient.disconnect();
    }
    delay(500);
    ESP.restart();
}

// Event für ioBroker (JSON)
void mqttPublishEvent(const String &eventName, const String &extraJson) {
    if (!mqttClient.connected()) return;
    String json = "{\"event\":\"" + eventName + "\",";
    json += "\"fw\":\"" + String(FW_VERSION) + "\",";
    json += "\"ts_uptime\":" + String(millis()/1000);
    if (extraJson.length() > 0) json += "," + extraJson;
    json += "}";
    mqttClient.publish(TOPIC_EVENT, json.c_str(), false);
    logInfo("Event sent: " + eventName);
}

static bool mqttReconnect() {
    String h = settingsGetMqttHost();
    int p = settingsGetMqttPort();
    mqttClient.setServer(h.c_str(), p);
    mqttClient.setCallback(mqttOnMessage);
    
    // Unique Client ID mit MAC
    String cid = String(MQTT_CLIENT_ID) + "-" + String((uint32_t)ESP.getEfuseMac(), HEX);
    
    // LWT setzen (Retained = true)
    if (mqttClient.connect(cid.c_str(), 0, 0, TOPIC_LWT, 1, true, MQTT_PAYLOAD_OFFLINE)) {
        logInfo("MQTT Connected");
        // Sofort Online melden
        mqttClient.publish(TOPIC_LWT, MQTT_PAYLOAD_ONLINE, true);
        
        mqttClient.subscribe(TOPIC_CMD);
        mqttClient.subscribe(TOPIC_CFG);
        
        // Boot Event (nur einmalig nach Connect)
        static bool bootSent = false;
        if (!bootSent) {
            mqttPublishEvent("boot");
            bootSent = true;
        }
        return true;
    }
    return false;
}

void mqttInit() { mqttClient.setBufferSize(512); }

void mqttLoop() {
    if (!wifiIsConnected()) return;
    
    if (!mqttClient.connected()) {
        unsigned long now = millis();
        if (now - lastMqttReconnectAttempt > 5000) {
            lastMqttReconnectAttempt = now;
            mqttReconnect();
        }
    } else {
        mqttClient.loop();
        // History Queue abarbeiten (wenn Puffer voll läuft)
        if (!historyQueue.empty()) {
            mqttClient.publish(TOPIC_HISTORY, historyQueue.front().c_str(), false);
            historyQueue.erase(historyQueue.begin());
        }
    }
}

void mqttSetCommandCallback(MqttCommandCallback cb) { commandCallback = cb; }

// === HIER WAR DER FEHLER ===
void mqttPublishState(const String &s) {
    if(mqttClient.connected()) {
        // ALT/FALSCH: mqttClient.publish(TOPIC_STATE, ("{\"state\":\""+s+"\"}").c_str(), true);
        
        // NEU/RICHTIG (für dein ioBroker Script): Einfach "OPEN" oder "CLOSED"
        mqttClient.publish(TOPIC_STATE, s.c_str(), true);
    }
}
// ===========================

void mqttPublishDiag(const String &m) {
    if(mqttClient.connected()) mqttClient.publish(TOPIC_DIAG, m.c_str());
}
void mqttPublishUsage(long u, long l) {
    if(mqttClient.connected()) {
        mqttClient.publish(TOPIC_USAGE, String(u).c_str(), true);
        mqttClient.publish(TOPIC_LIMIT, String(l).c_str(), true);
    }
}
void mqttLogDataPoint(String j) {
    if(mqttClient.connected()) mqttClient.publish(TOPIC_HISTORY, j.c_str(), false);
    else if(historyQueue.size() < 150) historyQueue.push_back(j);
}
void mqttPublish(const char* t, const char* p) {
    if(mqttClient.connected()) mqttClient.publish(t, p);
}