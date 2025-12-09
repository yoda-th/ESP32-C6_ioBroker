// Firmware V0.9.0 – based on V0.8.0, modified in this version.
//// web_module.cpp — Updated version with dynamic MQTT topic display
#include <Arduino.h>
#include "config.h"
#include "logger.h"
#include "wifi_module.h"
#include "mqtt_module.h"
#include "valve_module.h"
#include "flow_module.h"
#include "battery_module.h"
#include "web_module.h"
#include "time_module.h"
#include "watchdog_module.h"
#include "irrigation_module.h"

static unsigned long lastStatePublishMs = 0;
unsigned long lastOpenTimestamp = 0; // Zeitstempel (Sekunden seit 1970)
bool warningSent30h = false;
bool warningLeak = false;

String buildStateJson() {
    IrrigationProgram p = irrigationGetProgram();

    // V0.9 CHANGE: JSON konsistent mit Web-/API-Status
    String json = "{";
    json += "\"fw\":\"" + String(FW_VERSION) + "\",";
    json += "\"device\":\"" + String(DEVICE_NAME) + "\",";
    json += "\"wifi_ip\":\"" + wifiGetIp() + "\",";
    json += "\"wifi_rssi\":" + String(wifiGetRssi()) + ",";
    json += "\"valve\":\"" + String(valveGetState() == ValveState::OPEN ? "OPEN" : "CLOSED") + "\",";
    json += "\"flow_lpm\":" + String(flowGetLpm(), 2) + ",";
    json += "\"flow_total_l\":" + String(flowGetTotalLiters(), 2) + ",";
    json += "\"battery_v\":" + String(batteryGetVoltage(), 2) + ",";
    json += "\"irr_mode\":\"" + String(irrigationGetMode() == IrrigationMode::AUTO ? "AUTO" : "MANUAL") + "\",";
    json += "\"irr_running\":" + String(irrigationIsRunning() ? "true" : "false") + ",";
    json += "\"irr_remain_s\":" + String(irrigationGetRemainingSec()) + ",";
    json += "\"irr_start_h\":" + String(p.startHour) + ",";
    json += "\"irr_start_m\":" + String(p.startMinute) + ",";
    json += "\"irr_dur_s\":" + String(p.durationSec);
    json += "}";
    return json;
}

void onMqttCommand(const String &cmdJson) {
    logInfo("CMD: " + cmdJson);

    if (cmdJson.indexOf("OPEN") >= 0) {
        irrigationSetMode(IrrigationMode::MANUAL);
        valveSet(ValveState::OPEN);
    } else if (cmdJson.indexOf("CLOSE") >= 0) {
        irrigationSetMode(IrrigationMode::MANUAL);
        valveSet(ValveState::CLOSED);
    } else if (cmdJson.indexOf("MODE_AUTO") >= 0) {
        irrigationSetMode(IrrigationMode::AUTO);
    } else if (cmdJson.indexOf("MODE_MANUAL") >= 0) {
        irrigationSetMode(IrrigationMode::MANUAL);
    }
}

void setup() {
    logInit();
    logInfo("Boot " + String(DEVICE_NAME) + " FW " + FW_VERSION);

    pinMode(PIN_STATUS_LED, OUTPUT);
    digitalWrite(PIN_STATUS_LED, LOW);

    valveInit();
    flowInit();
    batteryInit();
    irrigationInit();

    wifiInit();
    timeInit();
    mqttInit();
    mqttSetCommandCallback(onMqttCommand);

    webInit();
    watchdogInit();

    // Initialisieren
    lastOpenTimestamp = time(NULL);

    logInfo("Setup done");
}

void loop() {
    watchdogFeed();

    wifiLoop();
    timeLoop();
    mqttLoop();
    flowLoop();
    batteryLoop();
    valveLoop();
    irrigationLoop();
    webLoop();
    watchdogLoop();

    // --- Millisekunden-Timer für MQTT und LED ---
    unsigned long nowMs = millis(); // Umbenannt in nowMs zur Klarheit

    if (nowMs - lastStatePublishMs >= 5000) {
        String stateJson = buildStateJson();
        mqttPublishState(stateJson);
        lastStatePublishMs = nowMs;
    }

    static bool led = false;
    static unsigned long lastLedMs = 0;
    if (nowMs - lastLedMs >= 1000) {
        led = !led;
        digitalWrite(PIN_STATUS_LED, led ? HIGH : LOW);
        lastLedMs = nowMs;
    }

    // --- LOGIK-BLOCK ---

    // 1. Zeitstempel aktualisieren (wenn Ventil offen)
    if (valveGetState() == ValveState::OPEN) {
        lastOpenTimestamp = time(NULL); // Zeit merken
        warningSent30h = false;         // Warnung Reset
    }

    // 2. Die 30-Stunden-Prüfung (Stagnations-Alarm)
    time_t nowEpoch = time(NULL); // FIX: Variable umbenannt (time_t vs unsigned long)
    
    if (nowEpoch > 10000) { // Nur wenn Zeit gültig (Systemuhr gestellt)
        // 30 Stunden = 108000 Sekunden
        if ((nowEpoch - lastOpenTimestamp) > 108000) {
            if (!warningSent30h) {
                String msg = "ALARM: Stagnation > 30h";
                logError(msg);
                mqttPublish(TOPIC_DIAG, msg.c_str());
                
                logSetLastDiag(msg); // <--- DAS IST NEU: Für Webseite speichern
                
                warningSent30h = true; 
            }
        }
    } // <--- FIX: Diese Klammer fehlte! (Schließt "if nowEpoch > 10000")

    // 3. Leckage-Schutz
    // Wenn Ventil ZU ist, aber Flow > 0.5 L/min
    if (valveGetState() == ValveState::CLOSED && flowGetLpm() > 0.5) {
        if (!warningLeak) {
            String msg = "ALARM: LEAK DETECTED!";
            logError(msg);
            mqttPublish(TOPIC_DIAG, msg.c_str());
            
            logSetLastDiag(msg); // <--- DAS IST NEU: Für Webseite speichern
            
            warningLeak = true;
        }
    } else {
        warningLeak = false;
    }
}