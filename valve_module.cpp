#include "valve_module.h"
#include "config.h"
#include "logger.h"
#include <esp_task_wdt.h> 

static ValveState currentState = ValveState::CLOSED;
static unsigned long dailyOpenSeconds = 0;
static unsigned long lastCountMs = 0;

// DEINE SPEZIELLE HARDWARE-LOGIK (Wiederhergestellt)
static void valveApplyHardware() {
    if (currentState == ValveState::OPEN) {
        // Öffnen: Pin als Output treiben + LOW ziehen
        pinMode(PIN_RELAY, OUTPUT);
        digitalWrite(PIN_RELAY, LOW); 
    } else {
        // Schließen: Pin hochohmig machen (Pullup)
        pinMode(PIN_RELAY, INPUT_PULLUP);
        // Zur Sicherheit High schreiben, falls er doch Output wird
        digitalWrite(PIN_RELAY, HIGH); 
    }
}

void valveInit() {
    // Startzustand: Zu (Input Pullup)
    pinMode(PIN_RELAY, INPUT_PULLUP);
    currentState = ValveState::CLOSED;
    valveApplyHardware(); 
    logInfo("Valve initialized (Low-Side/Input Mode)");
}

void valveLoop() {
    unsigned long now = millis();
    
    // Zähler Logik
    if (currentState == ValveState::OPEN) {
        if (now - lastCountMs >= 1000) {
            dailyOpenSeconds++;
            lastCountMs = now;
        }
    } else {
        lastCountMs = now; 
    }
}

void valveSet(ValveState s) {
    if (s != currentState) {
        currentState = s;
        valveApplyHardware();
        logInfo(String("Valve set to ") + (s == ValveState::OPEN ? "OPEN" : "CLOSED"));
    }
}

ValveState valveGetState() { return currentState; }

unsigned long valveGetDailyOpenSec() { return dailyOpenSeconds; }
void valveResetDailyOpenSec() { dailyOpenSeconds = 0; }

void valveSafeBeforeUpdate() {
    logWarn("Safe mode: Valve CLOSE for OTA");
    valveSet(ValveState::CLOSED);
    esp_task_wdt_delete(NULL); 
}

void valveSafeAfterUpdate() {
    logInfo("Safe mode done");
}