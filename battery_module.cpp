// Firmware V0.9.0 – based on V0.8.0, modified in this version.
#include "battery_module.h"
#include "config.h"
#include "logger.h"

// Beispiel-Spannungsteiler R1 (oben) / R2 (unten)
static const float ADC_REF     = 3.3f;
static const int   ADC_MAX     = 4095;
static const float R1_kOhm     = 51.0f;
static const float R2_kOhm     = 10.0f;

static float lastVoltage = 0.0f;
static unsigned long lastReadMs = 0;

void batteryInit() {
    analogReadResolution(12);
    pinMode(PIN_BATTERY_ADC, INPUT);
    logInfo("Battery measurement initialized");
}

void batteryLoop() {
    unsigned long now = millis();
    if (now - lastReadMs >= 5000) {
        int raw = analogRead(PIN_BATTERY_ADC);

        // V0.9 CHANGE: Clamping des ADC-Rohwerts, um Ausreißer abzufangen
        if (raw < 0) {
            raw = 0;
        } else if (raw > ADC_MAX) {
            raw = ADC_MAX;
        }

        float u_adc = (float)raw * ADC_REF / (float)ADC_MAX;
        float factor = (R1_kOhm + R2_kOhm) / R2_kOhm;
        lastVoltage = u_adc * factor;

        lastReadMs = now;
    }
}

float batteryGetVoltage() {
    return lastVoltage;
}
