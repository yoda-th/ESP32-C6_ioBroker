#pragma once
#include <Arduino.h>
#include <time.h> // Wichtig für struct tm

void timeInit();
void timeLoop();

// Helper für neuen Logger
String timeGetStr();

// === LEGACY / KOMPATIBILITÄT (Damit Irrigation/Watchdog nicht meckern) ===
bool timeIsValid();
void timeGetLocal(struct tm &out); // <--- Das fehlte eben!
bool timeIsDailyResetTime();