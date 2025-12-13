#include "irrigation_module.h"
#include "config.h"
#include "logger.h"
#include "valve_module.h"
#include "flow_module.h" 
#include "time_module.h" 
#include <time.h> 
#include <Preferences.h>

static IrrigationMode currentMode = IrrigationMode::AUTO;
static bool isRunning = false;
static unsigned long runStartTime = 0;
static unsigned long runDuration = 0;

static IrrigationSlot slots[MAX_PROGRAM_SLOTS];
static Preferences prefs; 

void irrigationInit() {
    prefs.begin("irr-slots", true); 
    size_t len = prefs.getBytes("data", slots, sizeof(slots));
    prefs.end();

    if (len != sizeof(slots)) {
        logWarn("No valid slots found. Init defaults.");
        for (int i = 0; i < MAX_PROGRAM_SLOTS; i++) {
            slots[i].enabled = false;
            slots[i].startHour = 6;
            slots[i].startMinute = 0;
            slots[i].durationSec = 600;
            slots[i].weekDays = 127; 
        }
    } else {
        logInfo("Loaded " + String(MAX_PROGRAM_SLOTS) + " slots.");
    }
}

void irrigationLoop() {
    // 1. ANOMALIE CHECK
    if (!isRunning && valveGetState() == ValveState::CLOSED && flowGetLpm() > 0.2f) {
        static unsigned long lastWarn = 0;
        if (millis() - lastWarn > 30000) {
            logWarn("Anomaly: Flow detected while valve CLOSED!");
            lastWarn = millis();
        }
    }

    // 2. Timer Stop
    if (isRunning) {
        if (millis() - runStartTime >= runDuration * 1000UL) {
            logInfo("Timer finished. Stopping.");
            irrigationStop();
        }
        return; 
    }

    // 3. Automatik
    if (currentMode == IrrigationMode::AUTO) {
        time_t now;
        time(&now);
        
        if (now > 1577836800) {
            struct tm ti;
            localtime_r(&now, &ti); 

            for (int i = 0; i < MAX_PROGRAM_SLOTS; i++) {
                if (slots[i].enabled) {
                    // Check Day
                    if ((slots[i].weekDays >> ti.tm_wday) & 1) {
                        // Check Time
                        if (ti.tm_hour == slots[i].startHour && ti.tm_min == slots[i].startMinute) {
                            if (ti.tm_sec < 5) {
                                logInfo("Slot " + String(i+1) + " Triggered! (Day " + String(ti.tm_wday) + ")");
                                irrigationStart(slots[i].durationSec);
                                delay(1000); 
                                return; 
                            }
                        }
                    }
                }
            }
        }
    } else {
        // MANUAL MODE WARNUNG (nur zur vollen Minute, zum Debuggen)
        static int lastLogMin = -1;
        time_t now; time(&now);
        struct tm ti; localtime_r(&now, &ti);
        if (ti.tm_min != lastLogMin && ti.tm_sec < 5) {
             // Wenn wir im Manual Mode sind und ein Slot eigentlich laufen würde, loggen wir das.
             // Das hilft bei "Warum geht es nicht?"
             lastLogMin = ti.tm_min;
        }
    }
}

void irrigationStart(int durationSec) {
    if (durationSec > 3600) durationSec = 3600; 
    valveSet(ValveState::OPEN);
    isRunning = true;
    runDuration = durationSec;
    runStartTime = millis();
}

void irrigationStop() {
    valveSet(ValveState::CLOSED);
    isRunning = false;
    currentMode = IrrigationMode::AUTO; // Timer fertig -> Zurück zu Auto!
}

void irrigationSetMode(IrrigationMode mode) {
    currentMode = mode;
}

IrrigationMode irrigationGetMode() {
    return currentMode;
}

bool irrigationIsRunning() {
    return isRunning;
}

int irrigationGetRemainingSec() {
    if (!isRunning) return 0;
    unsigned long elapsed = (millis() - runStartTime) / 1000;
    if (elapsed >= runDuration) return 0;
    return runDuration - elapsed;
}

void irrigationGetSlots(IrrigationSlot* targetArray) {
    memcpy(targetArray, slots, sizeof(slots));
}

void irrigationUpdateSlot(int index, IrrigationSlot slot) {
    if (index >= 0 && index < MAX_PROGRAM_SLOTS) {
        slots[index] = slot;
    }
}

void irrigationSaveToFlash() {
    prefs.begin("irr-slots", false); 
    size_t written = prefs.putBytes("data", slots, sizeof(slots));
    prefs.end();
    logInfo("Saved Slots to Flash (" + String(written) + " bytes)");
}