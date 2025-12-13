#pragma once
#include <Arduino.h>

// Initialisierung
void settingsInit();
void settingsLoad(); 
void settingsSave(); 

// --- Getter & Setter ---

// Tageslimit in Sekunden
int settingsGetDailyLimitSec();
void settingsSetDailyLimitSec(int s);

// Batterie Min
float settingsGetBatMin();
void settingsSetBatMin(float v);

// Batterie Kalibrierungs-Faktor
float settingsGetBatFactor();
void settingsSetBatFactor(float f);

// NEU: Flow Kalibrierungs-Faktor (Impulse pro Liter)
float settingsGetFlowFactor();
void settingsSetFlowFactor(float f);

// --- MQTT Konfiguration ---
String settingsGetMqttHost();
void settingsSetMqttHost(const String& host);

int settingsGetMqttPort();
void settingsSetMqttPort(int port);

// === Automatischer Reboot ===
void settingsSetRebootHour(int h); // 0-23, oder -1 für aus
int settingsGetRebootHour();