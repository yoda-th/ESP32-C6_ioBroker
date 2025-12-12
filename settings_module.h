#pragma once
#include <Arduino.h>

void settingsInit();
void settingsLoad(); // Bleibt erhalten (Kompatibilität)
void settingsSave(); // Bleibt erhalten

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

// --- MQTT Konfiguration ---
String settingsGetMqttHost();
void settingsSetMqttHost(const String& host);

int settingsGetMqttPort();
void settingsSetMqttPort(int port);

// === NEU: Automatischer Reboot ===
void settingsSetRebootHour(int h); // 0-23, oder -1 für aus
int settingsGetRebootHour();