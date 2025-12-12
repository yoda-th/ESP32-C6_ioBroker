#pragma once
#include <Arduino.h>

// Anzahl der verfügbaren Programme
#define MAX_PROGRAM_SLOTS 6

// Ein einzelner Programm-Slot (Speicheroptimiert)
struct IrrigationSlot {
    bool enabled;           // Aktiv?
    uint8_t startHour;      // 0-23
    uint8_t startMinute;    // 0-59
    uint16_t durationSec;   // Dauer in Sekunden
    uint8_t weekDays;       // Bitmaske: Bit 0=So, 1=Mo, ..., 6=Sa (127 = Alle Tage)
};

enum class IrrigationMode {
    AUTO,   // Zeitplan aktiv
    MANUAL  // Manuell gestartet
};

void irrigationInit();
void irrigationLoop();

// Steuerung
void irrigationStart(int durationSec); 
void irrigationStop();
void irrigationSetMode(IrrigationMode mode);

// Info
IrrigationMode irrigationGetMode();
bool irrigationIsRunning();
int irrigationGetRemainingSec();

// === SLOT MANAGEMENT (NEU) ===
// Gibt das gesamte Array zurück (für Web-Anzeige)
void irrigationGetSlots(IrrigationSlot* targetArray);

// Speichert einen einzelnen Slot (im RAM)
void irrigationUpdateSlot(int index, IrrigationSlot slot);

// Schreibt alles in den Flash (nur 1x aufrufen nach Änderungen)
void irrigationSaveToFlash();