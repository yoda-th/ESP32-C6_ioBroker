#pragma once
#include <Arduino.h>

enum class ValveState { CLOSED, OPEN };

void valveInit();
void valveLoop();
void valveSet(ValveState s);
ValveState valveGetState();

void valveSafeBeforeUpdate();
void valveSafeAfterUpdate();
