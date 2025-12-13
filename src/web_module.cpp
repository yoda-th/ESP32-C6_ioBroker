/****************************************************
 * web_module.cpp
 * Firmware: V1.2.9 - Timezone & Mode Display Fix
 ****************************************************/

#include "web_module.h"
#include "config.h"
#include "logger.h"
#include "wifi_module.h"
#include "valve_module.h"
#include "flow_module.h"
#include "battery_module.h"
#include "irrigation_module.h"
#include "settings_module.h" 
#include "mqtt_module.h" 
#include "time_module.h"

#include <WebServer.h>
#include <Update.h>

static WebServer server(80);

const String dayNames[] = {"Su","Mo","Tu","We","Th","Fr","Sa"};

const char* COMMON_CSS = 
"<style>"
"body { font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, Helvetica, Arial, sans-serif; margin: 0; padding: 10px; background: #f4f4f9; color: #333; max-width: 800px; margin: 0 auto; }"
"h1 { font-size: 1.5rem; margin-bottom: 5px; color: #444; }"
".header-info { background: #e9ecef; padding: 10px; border-radius: 5px; margin-bottom: 15px; display: flex; justify-content: space-between; align-items: center; font-weight: bold; font-family: monospace; font-size: 1.1rem; }"
"h2 { font-size: 1.2rem; margin-top: 20px; border-bottom: 2px solid #ddd; padding-bottom: 5px; }"
"p { margin: 5px 0; font-size: 0.95rem; line-height: 1.4; }"
".card { background: white; padding: 15px; border-radius: 8px; box-shadow: 0 2px 5px rgba(0,0,0,0.05); margin-bottom: 15px; }"
".btn { display: inline-block; padding: 12px 20px; font-size: 16px; font-weight: bold; color: white; text-decoration: none; border-radius: 5px; border: none; cursor: pointer; text-align: center; -webkit-appearance: none; margin: 5px 0; }"
".btn-green { background-color: #28a745; width: 48%; }"
".btn-red { background-color: #dc3545; width: 48%; }"
".btn-blue { background-color: #007bff; width: 100%; margin-top: 10px; }"
".btn-orange { background-color: #fd7e14; width: 100%; margin-top: 5px; }"
".btn-gray { background-color: #6c757d; font-size: 14px; padding: 8px 12px; }"
"input[type=number], input[type=text] { padding: 10px; font-size: 16px; border: 1px solid #ccc; border-radius: 4px; width: 60px; text-align: center; margin: 2px; }"
"input[type=checkbox] { transform: scale(1.5); margin: 5px; }"
"table { width: 100%; border-collapse: collapse; margin-top: 10px; }"
"th { text-align: left; background: #eee; padding: 10px; }"
"td { padding: 10px 5px; border-bottom: 1px solid #eee; vertical-align: middle; }"
".day-label { display: inline-block; padding: 6px 8px; background: #eef; border-radius: 4px; margin: 2px; font-size: 12px; cursor: pointer; }"
".nav { margin-top: 20px; padding-top: 10px; border-top: 1px solid #ccc; text-align: center; }"
".nav a { color: #007bff; text-decoration: none; margin: 0 8px; font-weight: bold; font-size: 14px; display:inline-block; padding:5px; }"
".progress-bg { background-color: #e9ecef; border-radius: 5px; height: 20px; width: 100%; margin: 10px 0; overflow: hidden; }"
".progress-fill { height: 100%; text-align: center; color: white; font-size: 12px; line-height: 20px; transition: width 0.5s; }"
".val { font-family: monospace; font-weight: bold; color: #0056b3; }"
"</style>";

String getNavFooter() {
    String h = "<div class='nav'>";
    h += "<a href='/'>Dashboard</a> | ";
    h += "<a href='/schedule'>Schedule</a> | ";
    h += "<a href='/mqtt_config'>MQTT</a> | ";
    h += "<a href='/diag'>Diag</a> | ";
    h += "<a href='/diag.json' target='_blank'>JSON</a> | ";
    h += "<a href='/update'>OTA</a>";
    h += "<br><br>";
    h += "<form action='/restart' method='POST'><input type='submit' value='Reboot Device' class='btn-gray'></form>";
    h += "</div>";
    return h;
}

static void addNoCacheHeaders() {
    server.sendHeader("Cache-Control", "no-store, no-cache, must-revalidate, max-age=0");
    server.sendHeader("Pragma", "no-cache");
    server.sendHeader("Expires", "0");
}

static bool checkAuth() {
    if (String(OTA_USER).length() == 0) return true;
    if (!server.authenticate(OTA_USER, OTA_PASS)) {
        server.requestAuthentication();
        return false;
    }
    return true;
}

static String buildDiagJson() {
    String j = "{";
    j += "\"fw\":\"" + String(FW_VERSION) + "\",";
    j += "\"uptime_s\":" + String(millis()/1000) + ",";
    j += "\"heap_free\":" + String(ESP.getFreeHeap()) + ",";
    j += "\"time_valid\":" + String(timeIsValid()?"true":"false") + ",";
    j += "\"vbat\":" + String(batteryGetVoltage(), 2) + ",";
    j += "\"vbat_raw\":" + String(batteryGetRawValue(), 3) + ",";
    j += "\"flow_lpm\":" + String(flowGetLpm(), 2) + ",";
    j += "\"valve\":\"" + String(valveGetState()==ValveState::OPEN?"OPEN":"CLOSED") + "\",";
    j += "\"mqtt_queue\":" + String(mqttGetQueueSize());
    j += "}";
    return j;
}

static void handleDiag() {
    if (!checkAuth()) return;
    addNoCacheHeaders();

    String html;
    html.reserve(6000); 
    html += F("<!DOCTYPE html><html><head><meta name='viewport' content='width=device-width, initial-scale=1'>");
    html += COMMON_CSS; 
    html += F("</head><body><h1>Deep Diagnostics</h1>");

    html += F("<div class='card'><h2>System & Time</h2>");
    html += "FW Version: <span class='val'>" + String(FW_VERSION) + "</span><br>";
    html += "Local Time: <span class='val'>" + timeGetStr() + "</span>";
    html += timeIsValid() ? " <b style='color:green'>(NTP OK)</b><br>" : " <b style='color:red'>(NOT SYNCED)</b><br>";
    html += "Irrigation Mode: <span class='val'>" + String(irrigationGetMode()==IrrigationMode::AUTO ? "AUTO" : "MANUAL") + "</span><br>";
    html += "Uptime: <span class='val'>" + String(millis()/1000) + " s</span><br>";
    html += "Free Heap: <span class='val'>" + String(ESP.getFreeHeap()) + " bytes</span><br>";
    html += "Last Diag Msg: <b>" + logGetLastDiag() + "</b>";
    html += F("</div>");

    html += F("<div class='card'><h2>MQTT Internals</h2>");
    html += "Connected: ";
    html += mqttIsConnected() ? "<b style='color:green'>YES</b><br>" : "<b style='color:red'>NO</b><br>";
    html += "State Code: <span class='val'>" + mqttGetStateString() + "</span><br>";
    html += "Last Error: <span class='val'>" + mqttGetLastError() + "</span><br>";
    html += "Outbound Queue: <span class='val'>" + String(mqttGetQueueSize()) + " items</span><br>";
    unsigned long recAge = mqttGetLastReconnectMs();
    String recStr = (recAge == 0) ? "Never" : (String(recAge/1000) + " s ago");
    html += "Last Attempt: <span class='val'>" + recStr + "</span>";
    html += F("</div>");

    html += F("<div class='card'><h2>Logs</h2><table style='font-size:12px;color:#004488;'>");
    html += logGetFlowEventsHtml(); 
    html += F("</table><br><table style='font-size:12px;'>");
    html += logGetEventsHtml(); 
    html += F("</table></div>");
    
    html += getNavFooter();
    html += F("</body></html>");
    server.send(200, "text/html", html);
}

// === API JSON HANDLER ===
static void handleDiagJson() {
    if (!checkAuth()) return;
    addNoCacheHeaders();
    server.send(200, "application/json", buildDiagJson());
}

static String buildStatusJson() {
    String json = "{";
    json += "\"fw\":\"" + String(FW_VERSION) + "\",";
    json += "\"device\":\"" + String(DEVICE_NAME) + "\",";
    json += "\"wifi_ip\":\"" + wifiGetIp() + "\",";
    json += "\"wifi_rssi\":" + String(wifiGetRssi()) + ",";
    json += "\"valve\":\"" + String(valveGetState() == ValveState::OPEN ? "OPEN" : "CLOSED") + "\",";
    json += "\"flow_lpm\":" + String(flowGetLpm(), 2) + ",";
    json += "\"flow_total_l\":" + String(flowGetTotalLiters(), 2) + ",";
    json += "\"battery_v\":" + String(batteryGetVoltage(), 2) + ",";
    json += "\"last_diag\":\"" + logGetLastDiag() + "\",";
    json += "\"daily_limit_sec\":" + String(settingsGetDailyLimitSec()) + ",";
    json += "\"daily_usage_sec\":" + String(valveGetDailyOpenSec()) + ",";
    json += "\"irr_mode\":\"" + String(irrigationGetMode() == IrrigationMode::AUTO ? "AUTO" : "MANUAL") + "\",";
    json += "\"irr_running\":" + String(irrigationIsRunning() ? "true" : "false") + ",";
    IrrigationSlot slots[MAX_PROGRAM_SLOTS];
    irrigationGetSlots(slots);
    int activeCount = 0;
    for(int i=0; i<MAX_PROGRAM_SLOTS; i++) if(slots[i].enabled) activeCount++;
    json += "\"active_slots\":" + String(activeCount) + ",";
    #define GENERATE_JSON_ITEM(name, path, label) json += "\"mqtt_" #name "\":\"" + String(TOPIC_##name) + "\",";
    MQTT_TOPIC_GENERATOR(GENERATE_JSON_ITEM)
    json += "\"meta\":\"auto_generated\"}";
    return json;
}

static void handleRoot() {
    if (!checkAuth()) return;
    addNoCacheHeaders();
    String html = "<html><head><title>" DEVICE_NAME "</title><meta name='viewport' content='width=device-width, initial-scale=1'>" + String(COMMON_CSS) + "</head><body>";
    html += "<h1>" DEVICE_NAME " (" FW_VERSION ")</h1>";
    
    // === NEU: HEADER MIT ZEIT UND MODE ===
    String modeStr = (irrigationGetMode() == IrrigationMode::AUTO) ? "<span style='color:green'>AUTO</span>" : "<span style='color:orange'>MANUAL</span>";
    html += "<div class='header-info'><span>" + timeGetStr() + "</span><span>" + modeStr + "</span></div>";

    html += "<div class='card'><h2>Status</h2>";
    html += "<p>IP: <b>" + wifiGetIp() + "</b> | RSSI: " + String(wifiGetRssi()) + " dBm</p>";
    String vColor = (valveGetState() == ValveState::OPEN ? "green" : "black");
    html += "<p>Valve: <b style='color:" + vColor + "; font-size:1.2rem;'>" + (valveGetState() == ValveState::OPEN ? "OPEN" : "CLOSED") + "</b></p>";
    html += "<p>Flow: <b>" + String(flowGetLpm(), 2) + " L/min</b> | Total: " + String(flowGetTotalLiters(), 1) + " L</p>";
    html += "<p>Battery: <b>" + String(batteryGetVoltage(), 2) + " V</b></p>";
    
    String diag = logGetLastDiag();
    if (diag != "OK") { 
        html += "<div style='background:#fdd; padding:10px; border-left:5px solid red; margin:10px 0;'>";
        html += "<b>" + diag + "</b>";
        html += "<form method='POST' action='/clear_diag' style='margin-top:5px;'><input type='submit' class='btn-gray' value='Acknowledge'></form></div>";
    } else {
        html += "<p style='color:green;'>" + diag + "</p>";
    }
    html += "</div>";

    html += "<div class='card'><h2>Manual Control</h2>";
    
    // === NEU: BUTTON UM MODE AUF AUTO ZU SETZEN ===
    if (irrigationGetMode() == IrrigationMode::MANUAL) {
        html += "<form method='POST' action='/set_auto'><input type='submit' class='btn btn-orange' value='&#8635; Reset to AUTO Mode'></form>";
    }
    
    html += "<form method='POST' action='/valve'>";
    html += "<button name='state' value='open' class='btn btn-green'>OPEN</button> <button name='state' value='close' class='btn btn-red'>CLOSE</button></form></div>";
    
    html += "<a href='/schedule' class='btn btn-blue' style='padding:15px 0; font-size:18px;'>&#128197; Configure Schedule</a>";

    unsigned long used = valveGetDailyOpenSec();
    unsigned long limit = settingsGetDailyLimitSec();
    int percent = 0; if (limit > 0) percent = (used * 100) / limit; if (percent > 100) percent = 100;
    String barColor = (percent > 90) ? "#dc3545" : "#28a745";
    html += "<div class='card'><h2>Usage & Settings</h2>";
    html += "<p>Daily Usage: <b>" + String(used/60) + " min</b> / " + String(limit/60) + " min</p>";
    html += "<div class='progress-bg'><div class='progress-fill' style='width:" + String(percent) + "%; background-color:" + barColor + ";'>" + String(percent) + "%</div></div>";
    html += "<form method='POST' action='/settings'>";
    html += "<p>Daily Limit (min):</p><input type='number' name='limit_min' value='" + String(settingsGetDailyLimitSec()/60) + "'>";
    html += "<p>Auto Reboot Hour (-1 = Off):</p><input type='number' name='reb_h' value='" + String(settingsGetRebootHour()) + "'>";
    html += "<p>Battery Min (V) / Factor:</p><input type='number' step='0.1' name='bat_min' value='" + String(settingsGetBatMin(),1) + "'> / <input type='number' step='0.01' name='bat_factor' value='" + String(settingsGetBatFactor(),2) + "'>";
    html += "<br><br><input type='submit' class='btn btn-blue' value='Save Settings'></form></div>";
    
    html += getNavFooter();
    html += "</body></html>";
    server.send(200, "text/html", html);
}

// ... (Restliche Handler bleiben gleich)

static void handleScheduleGet() {
    if (!checkAuth()) return;
    addNoCacheHeaders();
    IrrigationSlot slots[MAX_PROGRAM_SLOTS];
    irrigationGetSlots(slots);
    String html = "<html><head><title>Schedule</title><meta name='viewport' content='width=device-width, initial-scale=1'>" + String(COMMON_CSS) + "</head><body>";
    html += "<h1><a href='/' style='text-decoration:none; color:#333;'>&#8592;</a> Irrigation Schedule</h1>";
    // Header Info auch hier
    String modeStr = (irrigationGetMode() == IrrigationMode::AUTO) ? "<span style='color:green'>AUTO</span>" : "<span style='color:orange'>MANUAL</span>";
    html += "<div class='header-info'><span>" + timeGetStr() + "</span><span>" + modeStr + "</span></div>";

    html += "<form method='POST' action='/schedule_save'><div class='card' style='overflow-x:auto;'><table><tr><th>#</th><th>En</th><th>Time</th><th>Dur(s)</th><th>Days</th></tr>";
    for(int i=0; i<MAX_PROGRAM_SLOTS; i++) {
        String base = String(i);
        html += "<tr" + String(i%2==0 ? " style='background:#fafafa;'" : "") + "><td><b>" + String(i+1) + "</b></td>";
        html += "<td><input type='checkbox' name='en_" + base + "' " + (slots[i].enabled ? "checked" : "") + "></td>";
        html += "<td style='white-space:nowrap;'><input type='number' name='h_" + base + "' min='0' max='23' value='" + String(slots[i].startHour) + "' style='width:45px'>:<input type='number' name='m_" + base + "' min='0' max='59' value='" + String(slots[i].startMinute) + "' style='width:45px'></td>";
        html += "<td><input type='number' name='dur_" + base + "' value='" + String(slots[i].durationSec) + "' style='width:60px'></td>";
        html += "<td>";
        for(int d=1; d<=7; d++) {
            int bitIndex = (d == 7) ? 0 : d; 
            bool isSet = (slots[i].weekDays >> bitIndex) & 1;
            html += "<label class='day-label' style='" + String(isSet ? "background:#cce5ff;border:1px solid #004085;" : "background:#eee;color:#888;") + "'>";
            html += "<input type='checkbox' name='wd_" + base + "_" + String(bitIndex) + "' " + (isSet ? "checked" : "") + " style='margin:0; vertical-align:middle;'> " + dayNames[bitIndex] + "</label>";
            if(d==4) html += "<br>";
        }
        html += "</td></tr>";
    }
    html += "</table></div><input type='submit' class='btn btn-blue' value='Save Schedule'></form>";
    html += getNavFooter();
    html += "</body></html>";
    server.send(200, "text/html", html);
}

// ... (Rest wie vorher)

static void handleSchedulePost() {
    if (!checkAuth()) return;
    for (int i = 0; i < MAX_PROGRAM_SLOTS; i++) {
        String base = String(i);
        if (server.hasArg("h_" + base)) {
            IrrigationSlot slot;
            slot.startHour = server.arg("h_" + base).toInt();
            slot.startMinute = server.arg("m_" + base).toInt();
            slot.durationSec = server.arg("dur_" + base).toInt();
            slot.enabled = (server.hasArg("en_" + base) && server.arg("en_" + base) == "on");
            slot.weekDays = 0;
            for (int d = 0; d < 7; d++) {
                String dParam = "wd_" + base + "_" + String(d);
                if (server.hasArg(dParam) && server.arg(dParam) == "on") slot.weekDays |= (1 << d);
            }
            irrigationUpdateSlot(i, slot);
        }
    }
    irrigationSaveToFlash();
    server.sendHeader("Location", "/schedule", true);
    server.send(302, "text/plain", "Saved");
}

static void handleRestart() {
    if (!checkAuth()) return;
    server.send(200, "text/plain", "Rebooting... Bye!");
    delay(500);
    mqttGracefulRestart(); 
}

static void handleSettingsPost() {
    if (!checkAuth()) return;
    if (server.hasArg("limit_min")) settingsSetDailyLimitSec(server.arg("limit_min").toInt() * 60); 
    if (server.hasArg("bat_min")) settingsSetBatMin(server.arg("bat_min").toFloat());
    if (server.hasArg("bat_factor")) settingsSetBatFactor(server.arg("bat_factor").toFloat());
    if (server.hasArg("reb_h")) settingsSetRebootHour(server.arg("reb_h").toInt());
    server.sendHeader("Location", "/", true);
    server.send(302, "text/plain", "Saved");
}

static void handleMqttConfig() {
    if (!checkAuth()) return;
    addNoCacheHeaders();
    String html = "<html><head><meta name='viewport' content='width=device-width, initial-scale=1'>" + String(COMMON_CSS) + "</head><body><h1>MQTT Setup</h1><div class='card'><form method='POST' action='/mqtt_settings'>";
    html += "Host:<br><input type='text' name='host' value='" + settingsGetMqttHost() + "' style='width:100%'><br><br>";
    html += "Port:<br><input type='number' name='port' value='" + String(settingsGetMqttPort()) + "' style='width:100px'><br><br>";
    html += "<input type='submit' class='btn btn-blue' value='Save & Reboot'></form></div>";
    html += getNavFooter();
    html += "</body></html>";
    server.send(200, "text/html", html);
}

static void handleMqttSettingsPost() {
    if (!checkAuth()) return;
    if (server.hasArg("host")) settingsSetMqttHost(server.arg("host"));
    if (server.hasArg("port")) settingsSetMqttPort(server.arg("port").toInt());
    logInfo("MQTT settings updated via Web.");
    server.send(200, "text/plain", "Saved. Rebooting...");
    delay(200);
    mqttGracefulRestart();
}

static void handleClearDiagPost() {
    if (!checkAuth()) return;
    logSetLastDiag("OK");
    server.sendHeader("Location", "/", true);
    server.send(302, "text/plain", "Redirecting");
}

static void handleValvePost() {
    if (!checkAuth()) return;
    if (server.hasArg("state")) {
        String s = server.arg("state");
        irrigationSetMode(IrrigationMode::MANUAL);
        if (s == "open") valveSet(ValveState::OPEN);
        else valveSet(ValveState::CLOSED);
    }
    server.sendHeader("Location", "/", true);
    server.send(302, "text/plain", "Redirecting");
}

// === NEU: SET AUTO HANDLER ===
static void handleSetAutoPost() {
    if (!checkAuth()) return;
    irrigationSetMode(IrrigationMode::AUTO);
    logInfo("Manual Override: Reset to AUTO Mode");
    server.sendHeader("Location", "/", true);
    server.send(302, "text/plain", "Redirecting");
}

static void handleApiStatus() {
    if (!checkAuth()) return;
    addNoCacheHeaders();
    server.send(200, "application/json", buildStatusJson());
}

static void handleUpdateGet() {
    if (!checkAuth()) return;
    addNoCacheHeaders();
    String html = "<html><body><h1>OTA Update</h1><form method='POST' action='/update' enctype='multipart/form-data'><input type='file' name='update'><input type='submit' value='Upload'></form></body></html>";
    server.send(200, "text/html", html);
}

static void handleUpdatePost() {
    if (!checkAuth()) return;
    HTTPUpload &upload = server.upload();
    if (upload.status == UPLOAD_FILE_START) {
        valveSafeBeforeUpdate();
        if (!Update.begin()) Update.printError(Serial);
    } else if (upload.status == UPLOAD_FILE_WRITE) {
        if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) Update.printError(Serial);
    } else if (upload.status == UPLOAD_FILE_END) {
        if (Update.end(true)) {
            server.send(200, "text/html", "Update OK. Rebooting...");
            valveSafeAfterUpdate();
            delay(1000);
            ESP.restart();
        } else {
            server.send(500, "text/plain", "Update failed");
        }
    } else if (upload.status == UPLOAD_FILE_ABORTED) {
        Update.end();
    }
}

void webInit() {
    server.on("/",           HTTP_GET,  handleRoot);
    server.on("/schedule",   HTTP_GET,  handleScheduleGet);
    server.on("/schedule_save", HTTP_POST, handleSchedulePost);
    server.on("/valve",      HTTP_POST, handleValvePost);
    server.on("/set_auto",   HTTP_POST, handleSetAutoPost); // NEU
    server.on("/settings",   HTTP_POST, handleSettingsPost);
    server.on("/mqtt_config",HTTP_GET,  handleMqttConfig);
    server.on("/mqtt_settings", HTTP_POST, handleMqttSettingsPost);
    server.on("/diag",       HTTP_GET,  handleDiag);
    server.on("/diag.json",  HTTP_GET,  handleDiagJson); 
    server.on("/restart",    HTTP_POST, handleRestart);
    server.on("/clear_diag", HTTP_POST, handleClearDiagPost);
    server.on("/api/status", HTTP_GET,  handleApiStatus);
    server.on("/update",     HTTP_GET,  handleUpdateGet);
    server.on("/update",     HTTP_POST, [](){}, handleUpdatePost);

    server.begin();
    logInfo("Web/OTA server started on port 80");
}

void webLoop() {
    server.handleClient();
}