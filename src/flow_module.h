#pragma once
#include <Arduino.h>

void flowInit();
void flowLoop();
float flowGetLpm();
float flowGetTotalLiters();

// NEU: Zähler & Save
unsigned long flowGetTotalPulses();
void flowSaveToFlash(); 

unsigned long flowGetLastPulseAgeMs();