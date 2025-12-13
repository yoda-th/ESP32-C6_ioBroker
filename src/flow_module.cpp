#include "flow_module.h"
#include "config.h"
#include "logger.h"

// DEINE FILTER PARAMETER
static const float PULSES_PER_LITER = 450.0f; 
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
    logInfo("Flow sensor initialized (Filtered)");
}

void flowLoop() {
    unsigned long now = millis();
    // 100ms Fenster
    if (now - lastCalcMs >= 100) {
        
        // FIX: Atomares Auslesen und Resetten
        noInterrupts();
        unsigned long pulses = pulseCount;
        pulseCount = 0;
        interrupts();

        // Overflow Schutz
        if (pulses > MAX_PULSES_PER_INTERVAL) {
            pulses = 0; 
        }

        // Berechnung
        float pulsesPerSec = (float)pulses * 10.0f; // Weil 100ms Fenster
        float litersPerSec = pulsesPerSec / PULSES_PER_LITER;
        float candidateLpm = litersPerSec * 60.0f;

        // Plausibilitäts-Filter
        if (candidateLpm > MAX_LPM_ABS) candidateLpm = MAX_LPM_ABS;
        
        // Sprung-Filter
        if (lastLpm > 0.1f) {
            float maxAllowed = lastLpm * MAX_LPM_STEP_FACTOR;
            if (candidateLpm > maxAllowed) candidateLpm = maxAllowed;
        }

        // Summieren
        totalLiters += litersPerSec * ((now - lastCalcMs) / 1000.0f);
        
        lastLpm = candidateLpm;
        lastCalcMs = now;
    }
}

float flowGetLpm() { return lastLpm; }
float flowGetTotalLiters() { return totalLiters; }

unsigned long flowGetLastPulseAgeMs() {
    if (lastPulseMicros == 0) return 0;
    return (micros() - lastPulseMicros) / 1000;
}