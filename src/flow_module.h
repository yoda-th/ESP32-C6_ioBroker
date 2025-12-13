#pragma once
#include <Arduino.h>

void flowInit();
void flowLoop();
float flowGetLpm();
float flowGetTotalLiters();
// NEU: Diagnose
unsigned long flowGetLastPulseAgeMs();