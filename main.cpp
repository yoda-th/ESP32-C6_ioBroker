// Firmware V0.9.0 – based on V0.8.0, modified in this version.
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

String buildStateJson() {
    IrrigationProgram p = irrigationGetProgram();

    // V0.9 CHANGE (F4): JSON konsistent mit Web-/API-Status
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
    logInfo("Boot " + String(DEVICE_NAME) + " FW " + FW_VERSION); // V0.9: Version sichtbar

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

    unsigned long now = millis();
    if (now - lastStatePublishMs >= 5000) {
        String stateJson = buildStateJson();
        mqttPublishState(stateJson);
        lastStatePublishMs = now;
    }

    static bool led = false;
    static unsigned long lastLedMs = 0;
    if (now - lastLedMs >= 1000) {
        led = !led;
        digitalWrite(PIN_STATUS_LED, led ? HIGH : LOW);
        lastLedMs = now;
    }
}
