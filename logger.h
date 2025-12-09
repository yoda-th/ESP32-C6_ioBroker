//0.9.9
#pragma once
#include <Arduino.h>

void logInit();
void logInfo(const String &msg);
void logWarn(const String &msg);
void logError(const String &msg);
void logDiag(const String &msg);
// === NEU: DIAGNOSE SPEICHER ===
void logSetLastDiag(String s);
String logGetLastDiag();