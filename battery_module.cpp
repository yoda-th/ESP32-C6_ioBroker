#include "battery_module.h"
#include "config.h"
#include "logger.h"
#include "mqtt_module.h"     
#include "settings_module.h" 

static float currentVoltage = 0.0;
static float currentRawVoltage = 0.0; // NEU: Speicher für Rohwert
static unsigned long lastMeasure = 0;
static bool lowBatWarningSent = false;

void batteryInit() {
    pinMode(PIN_BATTERY_ADC, INPUT);
    analogSetAttenuation(ADC_11db); 
    batteryLoop(); 
}

void batteryLoop() {
    unsigned long now = millis();
    
    // Messung alle 5 Sekunden (zum Testen), später 60s
    if (now - lastMeasure > 5000 || lastMeasure == 0) {
        lastMeasure = now;
        
        // 1. Raw Spannung am Pin messen
        uint32_t pinMv = analogReadMilliVolts(PIN_BATTERY_ADC);
        float pinVoltage = pinMv / 1000.0;
        
        // Rohwert speichern für Telemetrie
        currentRawVoltage = pinVoltage; 

        // 2. Faktor anwenden
        float factor = settingsGetBatFactor(); 
        float voltage = pinVoltage * factor; 
        
        // Glätten
        if (currentVoltage == 0) currentVoltage = voltage;
        else currentVoltage = (currentVoltage * 0.8) + (voltage * 0.2);
        
        // 3. Warnschwelle prüfen
        float limit = settingsGetBatMin();
        if (limit > 0) {
            if (currentVoltage < limit) {
                if (!lowBatWarningSent) {
                    String msg = "ALARM: LOW BATTERY (" + String(currentVoltage, 1) + "V)";
                    logWarn(msg);
                    mqttPublish(TOPIC_DIAG, msg.c_str());
                    logSetLastDiag(msg);
                    lowBatWarningSent = true;
                }
            } else if (currentVoltage > (limit + 0.2)) {
                lowBatWarningSent = false;
            }
        }
    }
}

float batteryGetVoltage() {
    return currentVoltage;
}

// NEU: Getter für den Rohwert
float batteryGetRawValue() {
    return currentRawVoltage;
}