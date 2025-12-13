// Firmware V1.2.7 – ESP-Valve-C6
// - 6-Slot Irrigation Schedule
// - Hybrid Logging (Text + JSON)
// - Safety Watchdog & Auto-Reboot

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
#include "settings_module.h" 
#include <time.h> 

static unsigned long lastStatePublishMs = 0;
unsigned long lastOpenTimestamp = 0; 
bool warningSent30h = false;
bool warningLeak = false;
bool limitWarningSent = false;

// === JSON BUILDER (TELEMETRIE) ===
String buildTeleJson() {
    time_t rawTime;
    time(&rawTime);
    String json = "{";
    json += "\"ts\":" + String((unsigned long)rawTime) + ","; 
    json += "\"fw\":\"" + String(FW_VERSION) + "\",";
    json += "\"valve\":\"" + String(valveGetState() == ValveState::OPEN ? "OPEN" : "CLOSED") + "\",";
    json += "\"flow_lpm\":" + String(flowGetLpm(), 2) + ",";
    json += "\"flow_total_l\":" + String(flowGetTotalLiters(), 2) + ",";
    json += "\"battery_v\":" + String(batteryGetVoltage(), 2) + ",";
    json += "\"irr_mode\":\"" + String(irrigationGetMode() == IrrigationMode::AUTO ? "AUTO" : "MANUAL") + "\",";
    json += "\"irr_running\":" + String(irrigationIsRunning() ? "true" : "false") + ",";
    json += "\"daily_open_s\":" + String(valveGetDailyOpenSec());
    json += "}";
    return json;
}

// === JSON BUILDER (HISTORY) ===
String buildHistoryJson(time_t timestamp) {
    String json = "{";
    json += "\"ts\":" + String((unsigned long)timestamp) + ","; 
    json += "\"flow_l_min\":" + String(flowGetLpm(), 2) + ",";
    json += "\"total_l\":" + String(flowGetTotalLiters(), 2) + ",";
    json += "\"vbat\":" + String(batteryGetVoltage(), 2) + ",";
    json += "\"valve\":\"" + String(valveGetState() == ValveState::OPEN ? "OPEN" : "CLOSED") + "\"";
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
    settingsInit(); 
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

    unsigned long nowMs = millis(); 

    // BOOT EVENT
    static bool bootEventSent = false;
    if (!bootEventSent && mqttIsConnected()) {
        mqttPublishEvent("boot"); 
        bootEventSent = true;
    }

    // 1. DATA LOGGING
    static unsigned long lastLogTime = 0;
    static ValveState lastValveForLog = ValveState::CLOSED;
    static float lastFlowTotal = 0; 
    bool triggerLog = false;
    
    if (nowMs - lastLogTime >= 60000) triggerLog = true;
    
    ValveState currentV = valveGetState();
    if (currentV != lastValveForLog) {
        triggerLog = true;
        if (currentV == ValveState::OPEN) {
            logFlowEvent("START Valve Open");
            mqttPublishEvent("valve_open");
            lastFlowTotal = flowGetTotalLiters(); 
        } else {
            float delta = flowGetTotalLiters() - lastFlowTotal;
            String msg = "STOP Valve Closed (Total: " + String(delta, 1) + " L)";
            logFlowEvent(msg);
            
            String extra = "\"last_run_l\":" + String(delta, 2);
            mqttPublishEvent("valve_close", extra);
        }
        lastValveForLog = currentV;
    }
    
    if (triggerLog) {
        time_t rawTime;
        time(&rawTime);
        if (rawTime > 1700000000) { 
            String histJson = buildHistoryJson(rawTime);
            mqttLogDataPoint(histJson);
            lastLogTime = nowMs;
        }
    }

    // 2. LIVE STATUS
    unsigned long teleInterval = 60000; 
    if (valveGetState() == ValveState::OPEN || flowGetLpm() > 0.0) teleInterval = 5000;

    if (nowMs - lastStatePublishMs >= teleInterval) {
        mqttPublishState((valveGetState() == ValveState::OPEN) ? "OPEN" : "CLOSED"); 
        String teleJson = buildTeleJson(); 
        mqttPublish(TOPIC_TELE, teleJson.c_str());
        mqttPublish(TOPIC_LWT, MQTT_PAYLOAD_ONLINE); 
        mqttPublishUsage(valveGetDailyOpenSec(), settingsGetDailyLimitSec());
        lastStatePublishMs = nowMs;
    }

    // 3. LED
    static bool led = false;
    static unsigned long lastLedMs = 0;
    if (nowMs - lastLedMs >= (wifiIsConnected() ? 1000 : 200)) {
        led = !led;
        digitalWrite(PIN_STATUS_LED, led ? HIGH : LOW);
        lastLedMs = nowMs;
    }

    // 4. TAGES-RESET
    static int lastDay = -1; 
    time_t rawTime = time(NULL);
    struct tm timeInfo;
    if (localtime_r(&rawTime, &timeInfo) && timeInfo.tm_year > (2020 - 1900)) {
        if (lastDay == -1) lastDay = timeInfo.tm_mday; 
        if (timeInfo.tm_mday != lastDay) {
            logInfo("Daily Reset.");
            valveResetDailyOpenSec();
            limitWarningSent = false;   
            lastDay = timeInfo.tm_mday; 
        }
    }

    // 5. SAFETY
    if (valveGetDailyOpenSec() > (unsigned long)settingsGetDailyLimitSec()) {
        if (valveGetState() == ValveState::OPEN) {
            logError("FAILSAFE: Limit Exceeded");
            valveSet(ValveState::CLOSED);
            irrigationSetMode(IrrigationMode::MANUAL);
            if (!limitWarningSent) {
                mqttPublishEvent("alarm_limit");
                limitWarningSent = true;
            }
        }
    }

    // Auto Reboot (Safe)
    static int lastCheckHour = -1;
    if (rawTime > 1700000000) {
        struct tm* ti = localtime(&rawTime);
        if (ti->tm_hour != lastCheckHour) {
            lastCheckHour = ti->tm_hour;
            int rebootH = settingsGetRebootHour();
            if (rebootH >= 0 && ti->tm_hour == rebootH) {
                if (valveGetState() == ValveState::CLOSED) {
                    logWarn("Auto-Reboot Triggered");
                    mqttPublishEvent("reboot_scheduled");
                    mqttGracefulRestart();
                }
            }
        }
    }
}