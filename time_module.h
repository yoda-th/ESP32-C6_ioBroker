#pragma once
#include <Arduino.h>

void timeInit();
void timeLoop();
bool timeIsValid();
void timeGetLocal(struct tm &out);
bool timeIsDailyResetTime();
