#include "irrigation_module.h"
#include "config.h"
#include "logger.h"
#include "valve_module.h"
#include "flow_module.h" // Wichtig für Anomaly Check
#include "time_module.h" 
#include <time.h> 
#include <Preferences.h>

static IrrigationMode currentMode = IrrigationMode::AUTO;
static bool isRunning = false;
static unsigned long runStartTime = 0;
static unsigned long runDuration = 0;

// DAS ARRAY: 6 Slots im RAM
static IrrigationSlot slots[MAX_PROGRAM_SLOTS];

static Preferences prefs; 

// Initialisierung: Slots aus Flash laden
void irrigationInit() {
    prefs.begin("irr-slots", true); // Read-only
    
    // Wir versuchen, den ganzen Block zu lesen
    size_t len = prefs.getBytes("data", slots, sizeof(slots));
    
    prefs.end();

    // Wenn noch keine Daten da sind oder Größe falsch -> Defaults setzen
    if (len != sizeof(slots)) {
        logWarn("No valid slots found. Init defaults.");
        for (int i = 0; i < MAX_PROGRAM_SLOTS; i++) {
            slots[i].enabled = false;
            slots[i].startHour = 6;
            slots[i].startMinute = 0;
            slots[i].durationSec = 600;
            slots[i].weekDays = 127; // 127 = Alle Tage (Mo-So)
        }
    } else {
        logInfo("Loaded " + String(MAX_PROGRAM_SLOTS) + " slots from Flash.");
    }
}

void irrigationLoop() {
    // 1. ANOMALIE CHECK (aus V0.9 übernommen)
    // Wenn Ventil ZU ist, aber Flow > 0.2 l/min -> Alarm/Warnung
    if (!isRunning && valveGetState() == ValveState::CLOSED && flowGetLpm() > 0.2f) {
        // Wir loggen das nur alle paar Sekunden, um Spam zu vermeiden
        static unsigned long lastWarn = 0;
        if (millis() - lastWarn > 30000) {
            logWarn("Anomaly: Flow detected while valve CLOSED!");
            lastWarn = millis();
        }
    }

    // 2. Manuelle Laufzeit prüfen (Auto-Stop)
    if (isRunning) {
        if (millis() - runStartTime >= runDuration * 1000UL) {
            logInfo("Timer finished. Stopping.");
            irrigationStop();
        }
        return; // Wenn läuft, starten wir nichts Neues
    }

    // 3. Automatik prüfen (Nur im AUTO Modus)
    if (currentMode == IrrigationMode::AUTO) {
        // Wir nutzen die neue time_module Logik
        // Da time_module.h keine struct tm liefert, nutzen wir direkt time()
        time_t now;
        time(&now);
        
        // Zeit muss gültig sein (> 2020)
        if (now > 1577836800) {
            struct tm ti;
            localtime_r(&now, &ti); // ti.tm_wday: 0=So, 1=Mo, ..., 6=Sa

            // Wir prüfen alle 6 Slots durch
            for (int i = 0; i < MAX_PROGRAM_SLOTS; i++) {
                
                // Ist der Slot aktiv?
                if (slots[i].enabled) {
                    
                    // Passt der Wochentag? (Bitmask Check)
                    // Bit 0 = Sonntag, Bit 1 = Montag ...
                    if ((slots[i].weekDays >> ti.tm_wday) & 1) {
                        
                        // Passt die Uhrzeit?
                        if (ti.tm_hour == slots[i].startHour && ti.tm_min == slots[i].startMinute) {
                            
                            // Nur in den ersten 5 Sekunden der Minute triggern
                            if (ti.tm_sec < 5) {
                                logInfo("Slot " + String(i+1) + " Triggered! (Day " + String(ti.tm_wday) + ")");
                                irrigationStart(slots[i].durationSec);
                                delay(1000); // Debounce
                                return; // Nur ein Start pro Loop
                            }
                        }
                    }
                }
            }
        }
    }
}

void irrigationStart(int durationSec) {
    // Safety Limit aus Config beachten
    // Wir holen uns den Wert aus settings_module (falls Limit eingestellt)
    // Da wir aber hier keinen Zugriff auf Settings haben (Zyklus!), nutzen wir ein Hard Limit oder Default
    if (durationSec > 3600) durationSec = 3600; // Hard Safety Max 1h
    
    valveSet(ValveState::OPEN);
    isRunning = true;
    runDuration = durationSec;
    runStartTime = millis();
}

void irrigationStop() {
    valveSet(ValveState::CLOSED);
    isRunning = false;
    currentMode = IrrigationMode::AUTO; 
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

// === SLOT MANAGEMENT ===

void irrigationGetSlots(IrrigationSlot* targetArray) {
    // Kopiert unser internes Array in das Ziel
    memcpy(targetArray, slots, sizeof(slots));
}

void irrigationUpdateSlot(int index, IrrigationSlot slot) {
    if (index >= 0 && index < MAX_PROGRAM_SLOTS) {
        slots[index] = slot;
        // Achtung: Speichert noch nicht im Flash! Erst bei SaveToFlash()
    }
}

void irrigationSaveToFlash() {
    prefs.begin("irr-slots", false); // Read-Write
    // Wir schreiben das ganze Array als einen Blob (effizient)
    size_t written = prefs.putBytes("data", slots, sizeof(slots));
    prefs.end();
    logInfo("Saved Slots to Flash. Bytes: " + String(written));
}