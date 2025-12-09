#pragma once
#include <Arduino.h>

enum class IrrigationMode { AUTO, MANUAL };

struct IrrigationProgram {
    bool  enabled;
    int   startHour;
    int   startMinute;
    uint32_t durationSec;
    uint32_t maxRunSec;
};

void irrigationInit();
void irrigationLoop();

void irrigationSetMode(IrrigationMode m);
IrrigationMode irrigationGetMode();
IrrigationProgram irrigationGetProgram();
void irrigationSetProgram(const IrrigationProgram &p);

bool irrigationIsRunning();
uint32_t irrigationGetRemainingSec();
