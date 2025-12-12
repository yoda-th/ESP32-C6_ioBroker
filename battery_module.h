#pragma once
#include <Arduino.h>

void batteryInit();
void batteryLoop();
float batteryGetVoltage();

// NEU: Funktion für den Rohwert hinzufügen
float batteryGetRawValue();
