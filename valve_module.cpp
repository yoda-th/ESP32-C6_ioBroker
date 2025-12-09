//JB 2025-08-09 Pin set to INPUT for Valve close, Firmware upload fix, off-Watchdog
#include "valve_module.h"
#include "config.h"
#include "logger.h"
#include <esp_task_wdt.h> 

static ValveState currentState = ValveState::CLOSED;

// FINALE LOGIK: LOW-TRIGGER (Invertiert)
// OPEN  -> Pin OUTPUT + LOW (0V) -> Relais AN
// CLOSE -> Pin INPUT (Hochohmig) -> Relais AUS

static void valveApplyHardware() {
    if (currentState == ValveState::OPEN) {
        // VENTIL AUF: Erst Mode, dann Write (Fix für roten Fehler)
        pinMode(PIN_RELAY, OUTPUT);
        digitalWrite(PIN_RELAY, LOW); 
    } else {
        // VENTIL ZU: Pin trennen (Input Mode)
        // Pullup verhindert "Geister-Schalten" durch Störungen
        pinMode(PIN_RELAY, INPUT_PULLUP);
        // digitalWrite(PIN_RELAY, HIGH); // Nicht nötig bei INPUT, aber sicher ist sicher
    }
}

void valveInit() {
    // START: Sicher ZU (Input Mode)
    pinMode(PIN_RELAY, INPUT_PULLUP);
    
    currentState = ValveState::CLOSED;
    valveApplyHardware(); 
    
    logInfo("Valve initialized (Low-Trigger / Clean GPIO), state=CLOSED");
}

void valveLoop() {}

void valveSet(ValveState s) {
    if (s != currentState) {
        currentState = s;
        valveApplyHardware();
        logInfo(String("Valve set to ") + (s == ValveState::OPEN ? "OPEN" : "CLOSED"));
    }
}

ValveState valveGetState() { return currentState; }

void valveSafeBeforeUpdate() {
    logWarn("Safe mode: DISABLE Watchdog for OTA & Close Valve");
    
    // 1. Ventil sicher schließen
    valveSet(ValveState::CLOSED);

    // 2. Den Watchdog für diesen Task KÜNDIGEN (De-Init)
    // Damit darf das Update so lange dauern, wie es will.
    esp_task_wdt_delete(NULL); 
}

void valveSafeAfterUpdate() {
    logInfo("Safe mode: valve remains CLOSED after update");
}