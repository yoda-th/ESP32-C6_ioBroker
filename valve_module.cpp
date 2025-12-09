#include "valve_module.h"
#include "config.h"
#include "logger.h"

static ValveState currentState = ValveState::CLOSED;

// Hilfsfunktion: Schreibt den Pin sicher
// LOW-TRIGGER RELAIS: 
// LOW  (GND)  = Relais AN  = Ventil AUF
// HIGH (3.3V) = Relais AUS = Ventil ZU
static void valveApplyHardware() {
    if (currentState == ValveState::OPEN) {
        // Ventil öffnen -> Relais anziehen lassen -> PIN LOW
        digitalWrite(PIN_RELAY, LOW);
    } else {
        // Ventil schließen -> Relais abfallen lassen -> PIN HIGH
        digitalWrite(PIN_RELAY, HIGH);
    }
}

void valveInit() {
    // WICHTIG: Erst definierten Pegel setzen, dann auf Output schalten!
    // Wir wollen beim Booten sicher ZU sein (Relais aus = HIGH).
    digitalWrite(PIN_RELAY, HIGH);
    pinMode(PIN_RELAY, OUTPUT);
    
    currentState = ValveState::CLOSED;
    // Zur Sicherheit nochmal explizit Logik anwenden
    valveApplyHardware(); 
    
    logInfo("Valve initialized (Low-Trigger logic), state=CLOSED");
}

void valveLoop() {
    // Platz für Timeout-Überwachung falls nötig
}

void valveSet(ValveState s) {
    if (s != currentState) {
        currentState = s;
        valveApplyHardware();
        logInfo(String("Valve set to ") + (s == ValveState::OPEN ? "OPEN" : "CLOSED"));
    }
}

ValveState valveGetState() {
    return currentState;
}

void valveSafeBeforeUpdate() {
    logWarn("Safe mode: closing valve before update");
    valveSet(ValveState::CLOSED);
    delay(500); // Zeit geben zum Relais-Schalten
}

void valveSafeAfterUpdate() {
    logInfo("Safe mode: valve remains CLOSED after update");
}