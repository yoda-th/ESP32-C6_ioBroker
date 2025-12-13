#pragma once
#include <Arduino.h>

// Typdefinition für den Callback
typedef void (*MqttCommandCallback)(const String &);

void mqttInit();
void mqttLoop();
void mqttSetCommandCallback(MqttCommandCallback callback);

// Standard Publish Funktionen
void mqttPublishState(const String &state);     // Sendet OPEN/CLOSED
void mqttPublishDiag(const String &msg);        // Sendet Text Diagnose
void mqttPublishUsage(long usageSec, long limitSec); // Sendet /usage und /limit
void mqttPublish(const char* topic, const char* payload);

// Data Logging (History)
void mqttLogDataPoint(String jsonPayload);

// === NEU (Wiederhergestellt): JSON Events ===
// Sendet strukturiertes Event an /event (für ioBroker Skripte)
void mqttPublishEvent(const String &eventName, const String &extraJson = "");

// Diagnose & Status Helper
String mqttGetStateString();    // "Connected", "Disconnected"...
String mqttGetLastError();      // Letzter Fehlergrund
bool   mqttIsConnected();       // Verbindung da?
void   mqttGracefulRestart();   // Sendet "Offline", wartet kurz, rebootet

// NEU: Diagnose Getter
size_t mqttGetQueueSize();
unsigned long mqttGetLastReconnectMs();