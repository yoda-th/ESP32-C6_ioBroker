#pragma once
#include <Arduino.h>

// Initialisierung
void logInit();

// Standard Logging (Serial + optional MQTT Debug)
void logInfo(const String &msg);
void logWarn(const String &msg);
void logError(const String &msg);
void logDiag(const String &msg);

// === DIAGNOSE SPEICHER (Letzter Status) ===
void logSetLastDiag(String s);
String logGetLastDiag();

// === Event Log (System/Error) - Ringpuffer 20 ===
// Für Fehler, WLAN-Probleme, Reboots
void logEvent(String type, String msg); 
String logGetEventsHtml();              

// === NEU: Valve & Flow Log - Ringpuffer 50 ===
// Speziell für: "Ventil Auf", "Ventil Zu (X Liter)"
void logFlowEvent(String msg);
String logGetFlowEventsHtml();