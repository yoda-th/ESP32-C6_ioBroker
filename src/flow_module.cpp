#include "flow_module.h"
#include "config.h"
#include "logger.h"
#include "settings_module.h" // NEU: Für K-Factor
#include <Preferences.h>

// RTC_DATA_ATTR behält Wert bei Reboot (aber nicht bei Stromausfall)
RTC_DATA_ATTR unsigned long rtcTotalPulses = 0;

static const float MAX_LPM_STEP_FACTOR = 5.0f;
static const float MAX_LPM_ABS         = 60.0f;
static const unsigned long MAX_PULSES_PER_INTERVAL = 20000;
static const unsigned long MIN_PULSE_SPACING_US    = 150;

static volatile unsigned long pulseCount = 0;
static unsigned long lastPulseMicros = 0;
static float lastLpm = 0.0f;
static float totalLiters = 0.0f;
static unsigned long lastCalcMs = 0;

void IRAM_ATTR flowIsr() {
    unsigned long nowUs = micros();
    if (nowUs - lastPulseMicros < MIN_PULSE_SPACING_US) return; 
    lastPulseMicros = nowUs;
    pulseCount++;
}

void flowInit() {
    pinMode(PIN_FLOW, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(PIN_FLOW), flowIsr, FALLING);
    lastCalcMs = millis();

    // Bei Stromausfall (RTC=0) versuchen, alten Stand aus Flash zu holen
    if (rtcTotalPulses == 0) {
        Preferences p;
        p.begin("flow-data", true); // Read-only
        unsigned long saved = p.getULong("pulses", 0);
        p.end();
        if (saved > 0) {
            rtcTotalPulses = saved;
            logInfo("Restored Flow Pulses from Flash: " + String(rtcTotalPulses));
        }
    }
    
    logInfo("Flow init. Total Pulses: " + String(rtcTotalPulses));
}

void flowLoop() {
    unsigned long now = millis();
    // 100ms Fenster
    if (now - lastCalcMs >= 100) {
        
        noInterrupts();
        unsigned long pulses = pulseCount;
        pulseCount = 0;
        interrupts();

        if (pulses > MAX_PULSES_PER_INTERVAL) pulses = 0; 
        
        // NEU: Zum Ewigen Zähler addieren
        if (pulses > 0) {
            rtcTotalPulses += pulses;
        }

        // Berechnung mit variablem Faktor aus Settings
        float kFactor = settingsGetFlowFactor();
        if (kFactor <= 0.1f) kFactor = 450.0f; // Fallback

        float pulsesPerSec = (float)pulses * 10.0f; 
        float litersPerSec = pulsesPerSec / kFactor;
        float candidateLpm = litersPerSec * 60.0f;

        // Filterung
        if (candidateLpm > MAX_LPM_ABS) candidateLpm = MAX_LPM_ABS;
        if (lastLpm > 0.1f) {
            float maxAllowed = lastLpm * MAX_LPM_STEP_FACTOR;
            if (candidateLpm > maxAllowed) candidateLpm = maxAllowed;
        }

        totalLiters += litersPerSec * ((now - lastCalcMs) / 1000.0f);
        lastLpm = candidateLpm;
        lastCalcMs = now;
    }
}

float flowGetLpm() { return lastLpm; }
float flowGetTotalLiters() { return totalLiters; }

// NEU: Getter für Rohdaten
unsigned long flowGetTotalPulses() {
    return rtcTotalPulses;
}

// NEU: Speichern in Flash (wird bei Ventil-Zu aufgerufen)
void flowSaveToFlash() {
    Preferences p;
    p.begin("flow-data", false);
    p.putULong("pulses", rtcTotalPulses);
    p.end();
    // Optional: Loggen, aber nicht zu oft spammen
    // logInfo("Saved pulses to Flash"); 
}

unsigned long flowGetLastPulseAgeMs() {
    if (lastPulseMicros == 0) return 0;
    return (micros() - lastPulseMicros) / 1000;
}