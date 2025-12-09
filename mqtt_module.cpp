#include "mqtt_module.h"
#include "config.h"
#include "logger.h"
#include "wifi_module.h"

#include <WiFi.h>
#include <PubSubClient.h>

static WiFiClient espClient;
static PubSubClient mqttClient(espClient);
static MqttCommandCallback commandCallback = nullptr;
static unsigned long lastMqttReconnectAttempt = 0;

static void mqttOnMessage(char* topic, byte* payload, unsigned int length) {
    String t(topic);
    String msg;
    msg.reserve(length+1);
    for (unsigned int i = 0; i < length; i++) {
        msg += (char)payload[i];
    }
    logInfo("MQTT message on " + t + ": " + msg);

    if (t == TOPIC_CMD) {
        if (commandCallback) {
            commandCallback(msg);
        }
    } else if (t == TOPIC_CFG) {
        logDiag("Received config: " + msg);
    } else if (t == TOPIC_PROG) {
        logDiag("Received program: " + msg);
        // TODO: Programm-Parsing und Übergabe an irrigation_module
    }
}

static bool mqttReconnect() {
    if (!wifiIsConnected()) return false;

    String host = wifiGetMqttHost();
    logInfo("Connecting MQTT to " + host);
    mqttClient.setServer(host.c_str(), MQTT_PORT);
    mqttClient.setCallback(mqttOnMessage);

// === LWT HINZUFÜGEN ===
    // Parameter: ID, User(null), Pass(null), WillTopic, QoS, Retain, WillMessage
    if (mqttClient.connect(MQTT_CLIENT_ID, nullptr, nullptr, TOPIC_LWT, 1, true, "Offline")) {
        
        logInfo("MQTT connected");
        
        // 1. Sofort "Online" melden (Retained = true)
        mqttClient.publish(TOPIC_LWT, "Online", true);

        // 2. Themen abonnieren
        mqttClient.subscribe(TOPIC_CMD);
        mqttClient.subscribe(TOPIC_CFG);
        mqttClient.subscribe(TOPIC_PROG);
        
        // 3. Config anfordern
        mqttPublishCfgRequest();
        
        return true;
    } else {
        // ... (Ihr bestehender Fehler-Code) ...
        logError("MQTT connect failed, rc=" + String(mqttClient.state()));
        return false;
    }
}

void mqttInit() {
    // tatsächliche Verbindung in mqttLoop
}

void mqttLoop() {
    if (!wifiIsConnected()) return;

    if (!mqttClient.connected()) {
        unsigned long now = millis();
        if (now - lastMqttReconnectAttempt > 5000) {
            lastMqttReconnectAttempt = now;
            mqttReconnect();
        }
        return;
    }
    mqttClient.loop();
}

void mqttSetCommandCallback(MqttCommandCallback cb) {
    commandCallback = cb;
}

void mqttPublishState(const String &json) {
    if (!mqttClient.connected()) return;
    mqttClient.publish(TOPIC_STATE, json.c_str(), true);
}

void mqttPublishDiag(const String &json) {
    if (!mqttClient.connected()) return;
    mqttClient.publish(TOPIC_DIAG, json.c_str(), false);
}

void mqttPublishCfgRequest() {
    if (!mqttClient.connected()) return;
    mqttClient.publish(TOPIC_CFG, "{\"request\":\"cfg\"}", false);
}
