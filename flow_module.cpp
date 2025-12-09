// Firmware V0.9.0 – based on V0.8.0, modified in this version.
#include "flow_module.h"
#include "config.h"
#include "logger.h"
#include <limits>

// V0.9 CHANGE: Verbesserte Flow-Logik mit 100-ms-Fenster, Plausibilitätsfilter & Overflow-Schutz

static volatile unsigned long pulseCount = 0;
static unsigned long lastPulseMicros = 0;   // V0.9 CHANGE: einfache Entprellung

static float lastLpm = 0.0f;
static float totalLiters = 0.0f;
static unsigned long lastCalcMs = 0;

// an Sensor anpassen:
static const float PULSES_PER_LITER = 450.0f;

// V0.9 CHANGE: Plausibilitätsparameter
static const float MAX_LPM_STEP_FACTOR = 5.0f;    // max. Sprungfaktor vs. vorher
static const float MAX_LPM_ABS         = 40.0f;   // harter Deckel
static const unsigned long MAX_PULSES_PER_INTERVAL = 20000; // Overflow-Schutz
static const unsigned long MIN_PULSE_SPACING_US    = 150;   // primitive Entprellung

void IRAM_ATTR flowIsr() {
    unsigned long nowUs = micros();
    // V0.9 CHANGE: einfache Entprellung – extrem schnelle Pulse ignorieren
    if (nowUs - lastPulseMicros < MIN_PULSE_SPACING_US) {
        return;
    }
    lastPulseMicros = nowUs;
    pulseCount++;
}

void flowInit() {
    pinMode(PIN_FLOW, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(PIN_FLOW), flowIsr, FALLING);
    lastCalcMs = millis();
    logInfo("Flow sensor initialized");
}

void flowLoop() {
    unsigned long now = millis();
    // V0.9 CHANGE: 100-ms-Updatefenster, aber weiterhin auf 1 s normalisiert
    if (now - lastCalcMs >= 100) {
        unsigned long pulses = pulseCount;
        pulseCount = 0;

        if (pulses > MAX_PULSES_PER_INTERVAL) {
            // V0.9 CHANGE: Overflow-/Fehler-Schutz
            logWarn("Flow pulses exceed MAX_PULSES_PER_INTERVAL, clamping");
            pulses = MAX_PULSES_PER_INTERVAL;
        }

        // Normierung auf Sekundenrate: pulses / 0.1 s = pulses * 10 pro Sekunde
        float pulsesPerSec = (float)pulses * 10.0f;
        float litersPerSec = pulsesPerSec / PULSES_PER_LITER;
        float candidateLpm = litersPerSec * 60.0f;

        // V0.9 CHANGE: Plausibilitätsfilter
        if (candidateLpm < 0.0f) {
            candidateLpm = 0.0f;
        }
        if (candidateLpm > MAX_LPM_ABS) {
            logWarn("Flow LPM above MAX_LPM_ABS, clamping");
            candidateLpm = MAX_LPM_ABS;
        }
        if (lastLpm > 0.1f) {
            float maxAllowed = lastLpm * MAX_LPM_STEP_FACTOR;
            if (candidateLpm > maxAllowed) {
                logWarn("Flow LPM jump too large, smoothing");
                candidateLpm = maxAllowed;
            }
        }

        // 100-ms-Intervall → Litermenge addieren
        totalLiters += litersPerSec * ((now - lastCalcMs) / 1000.0f);

        lastLpm = candidateLpm;
        lastCalcMs = now;
    }
}

float flowGetLpm() {
    return lastLpm;
}

float flowGetTotalLiters() {
    return totalLiters;
}
