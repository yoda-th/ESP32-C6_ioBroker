/****************************************************
 * web_module.cpp
 * Firmware-Linie: V1.x
 * Stand: V1.2.3 - Fix: Restore Progress Bar & API Link
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

#include <WebServer.h>
#include <Update.h>

static WebServer server(80);

const String dayNames[] = {"Su","Mo","Tu","We","Th","Fr","Sa"};

// CSS
const char* COMMON_CSS = 
"<style>"
"body { font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, Helvetica, Arial, sans-serif; margin: 0; padding: 10px; background: #f4f4f9; color: #333; max-width: 800px; margin: 0 auto; }"
"h1 { font-size: 1.5rem; margin-bottom: 10px; color: #444; }"
"h2 { font-size: 1.2rem; margin-top: 20px; border-bottom: 2px solid #ddd; padding-bottom: 5px; }"
"p { margin: 5px 0; font-size: 0.95rem; }"
".card { background: white; padding: 15px; border-radius: 8px; box-shadow: 0 2px 5px rgba(0,0,0,0.05); margin-bottom: 15px; }"
".btn { display: inline-block; padding: 12px 20px; font-size: 16px; font-weight: bold; color: white; text-decoration: none; border-radius: 5px; border: none; cursor: pointer; text-align: center; -webkit-appearance: none; margin: 5px 0; }"
".btn-green { background-color: #28a745; width: 48%; }"
".btn-red { background-color: #dc3545; width: 48%; }"
".btn-blue { background-color: #007bff; width: 100%; margin-top: 10px; }"
".btn-gray { background-color: #6c757d; font-size: 14px; padding: 8px 12px; }"
"input[type=number], input[type=text] { padding: 10px; font-size: 16px; border: 1px solid #ccc; border-radius: 4px; width: 60px; text-align: center; margin: 2px; }"
"input[type=checkbox] { transform: scale(1.5); margin: 5px; }"
"table { width: 100%; border-collapse: collapse; margin-top: 10px; }"
"th { text-align: left; background: #eee; padding: 10px; }"
"td { padding: 10px 5px; border-bottom: 1px solid #eee; vertical-align: middle; }"
".day-label { display: inline-block; padding: 6px 8px; background: #eef; border-radius: 4px; margin: 2px; font-size: 12px; cursor: pointer; }"
".nav { margin-top: 20px; padding-top: 10px; border-top: 1px solid #ccc; text-align: center; }"
".nav a { color: #007bff; text-decoration: none; margin: 0 10px; font-weight: bold; font-size: 14px; }"
/* Progress Bar Style */
".progress-bg { background-color: #e9ecef; border-radius: 5px; height: 20px; width: 100%; margin: 10px 0; overflow: hidden; }"
".progress-fill { height: 100%; text-align: center; color: white; font-size: 12px; line-height: 20px; transition: width 0.5s; }"
"</style>";

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

// JSON API
static String buildStatusJson() {
    String json = "{";
    json += "\"fw\":\"" + String(FW_VERSION) + "\",";
    json += "\"device\":\"" + String(DEVICE_NAME) + "\",";
    json += "\"wifi_ip\":\"" + wifiGetIp() + "\",";
    json += "\"valve\":\"" + String(valveGetState() == ValveState::OPEN ? "OPEN" : "CLOSED") + "\",";
    json += "\"flow_lpm\":" + String(flowGetLpm(), 2) + ",";
    json += "\"battery_v\":" + String(batteryGetVoltage(), 2) + ",";
    json += "\"daily_limit_sec\":" + String(settingsGetDailyLimitSec()) + ",";
    json += "\"daily_usage_sec\":" + String(valveGetDailyOpenSec()) + ","; // Hinzugefügt
    json += "\"reboot_hour\":" + String(settingsGetRebootHour()) + ",";
    json += "\"irr_mode\":\"" + String(irrigationGetMode() == IrrigationMode::AUTO ? "AUTO" : "MANUAL") + "\",";
    json += "\"irr_running\":" + String(irrigationIsRunning() ? "true" : "false") + "}";
    return json;
}

// === HAUPTSEITE (DASHBOARD) ===
static void handleRoot() {
    if (!checkAuth()) return;
    addNoCacheHeaders();

    String html = "<html><head><title>" DEVICE_NAME "</title>";
    html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
    html += COMMON_CSS;
    html += "</head><body>";

    html += "<h1>" DEVICE_NAME "</h1>";

    // 1. STATUS KARTE
    html += "<div class='card'>";
    html += "<h2>Status</h2>";
    html += "<p>IP: <b>" + wifiGetIp() + "</b> | RSSI: " + String(wifiGetRssi()) + " dBm</p>";
    
    String vState = (valveGetState() == ValveState::OPEN ? "OPEN" : "CLOSED");
    String vColor = (valveGetState() == ValveState::OPEN ? "green" : "black");
    html += "<p>Valve: <b style='color:" + vColor + "; font-size:1.2rem;'>" + vState + "</b></p>";
    
    html += "<p>Flow: <b>" + String(flowGetLpm(), 2) + " L/min</b></p>";
    html += "<p>Battery: <b>" + String(batteryGetVoltage(), 2) + " V</b></p>";

    // Warnungen
    String diag = logGetLastDiag();
    if (diag.startsWith("ALARM")) {
        html += "<div style='background:#fdd; padding:10px; border-left:5px solid red; margin:10px 0;'>";
        html += "<b>" + diag + "</b>";
        html += "<form method='POST' action='/clear_diag' style='margin-top:5px;'><input type='submit' class='btn-gray' value='Acknowledge'></form></div>";
    } else {
        html += "<p style='color:green;'>" + diag + "</p>";
    }
    html += "</div>";

    // 2. CONTROL KARTE
    html += "<div class='card'>";
    html += "<h2>Manual Control</h2>";
    html += "<form method='POST' action='/valve'>";
    html += "<button name='state' value='open' class='btn btn-green'>OPEN</button> ";
    html += "<button name='state' value='close' class='btn btn-red'>CLOSE</button>";
    html += "</form>";
    html += "</div>";

    // 3. SCHEDULE LINK
    html += "<a href='/schedule' class='btn btn-blue' style='padding:15px 0; font-size:18px;'>&#128197; Configure Schedule</a>";

    // 4. SETTINGS & LIMIT KARTE (Hier ist der Balken zurück!)
    html += "<div class='card'>";
    html += "<h2>Usage & Settings</h2>";
    
    // --- FORTSCHRITTSBALKEN START ---
    unsigned long used = valveGetDailyOpenSec();
    unsigned long limit = (unsigned long)settingsGetDailyLimitSec();
    int percent = 0;
    if (limit > 0) percent = (used * 100) / limit;
    if (percent > 100) percent = 100;
    
    String barColor = (percent > 90) ? "#dc3545" : "#28a745"; // Rot bei >90%, sonst Grün

    html += "<p>Daily Usage: <b>" + String(used/60) + " min</b> / " + String(limit/60) + " min</p>";
    html += "<div class='progress-bg'>";
    html += "<div class='progress-fill' style='width:" + String(percent) + "%; background-color:" + barColor + ";'>" + String(percent) + "%</div>";
    html += "</div>";
    // --- FORTSCHRITTSBALKEN ENDE ---

    html += "<form method='POST' action='/settings'>";
    html += "<p>Daily Limit (min):</p>";
    html += "<input type='number' name='limit_min' value='" + String(settingsGetDailyLimitSec()/60) + "'>";
    
    html += "<p>Auto Reboot Hour (-1 = Off):</p>";
    html += "<input type='number' name='reb_h' value='" + String(settingsGetRebootHour()) + "'>";

    html += "<p>Battery Min (V) / Factor:</p>";
    html += "<input type='number' step='0.1' name='bat_min' value='" + String(settingsGetBatMin(),1) + "'> / ";
    html += "<input type='number' step='0.01' name='bat_factor' value='" + String(settingsGetBatFactor(),2) + "'>";

    html += "<br><br><input type='submit' class='btn btn-blue' value='Save Settings'>";
    html += "</form>";
    html += "</div>";

    // FOOTER LINKS (API Link wieder da)
    html += "<div class='nav'>";
    html += "<a href='/schedule'>Schedule</a> | ";
    html += "<a href='/mqtt_config'>MQTT</a> | ";
    html += "<a href='/diag'>Diag</a> | ";
    html += "<a href='/api/status'>JSON API</a> | ";  // <--- HIER
    html += "<a href='/update'>Update</a>";
    html += "<br><br>";
    html += "<form action='/restart' method='POST'><input type='submit' value='Reboot Device' class='btn-gray' onclick=\"return confirm('Reboot?');\"></form>";
    html += "</div>";

    html += "</body></html>";
    server.send(200, "text/html", html);
}

// === SCHEDULE SEITE (/schedule) ===
static void handleScheduleGet() {
    if (!checkAuth()) return;
    addNoCacheHeaders();

    IrrigationSlot slots[MAX_PROGRAM_SLOTS];
    irrigationGetSlots(slots);

    String html = "<html><head><title>Schedule</title>";
    html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
    html += COMMON_CSS;
    html += "</head><body>";

    html += "<h1><a href='/' style='text-decoration:none; color:#333;'>&#8592;</a> Irrigation Schedule</h1>";
    html += "<form method='POST' action='/schedule_save'>";
    html += "<div class='card' style='overflow-x:auto;'>"; 
    html += "<table>";
    html += "<tr><th>#</th><th>En</th><th>Time</th><th>Dur(s)</th><th>Days</th></tr>";

    for(int i=0; i<MAX_PROGRAM_SLOTS; i++) {
        String base = String(i);
        html += "<tr" + String(i%2==0 ? " style='background:#fafafa;'" : "") + ">";
        html += "<td><b>" + String(i+1) + "</b></td>";
        html += "<td><input type='checkbox' name='en_" + base + "' " + (slots[i].enabled ? "checked" : "") + "></td>";
        html += "<td style='white-space:nowrap;'>";
        html += "<input type='number' name='h_" + base + "' min='0' max='23' value='" + String(slots[i].startHour) + "' style='width:45px'>";
        html += ":";
        html += "<input type='number' name='m_" + base + "' min='0' max='59' value='" + String(slots[i].startMinute) + "' style='width:45px'>";
        html += "</td>";
        html += "<td><input type='number' name='dur_" + base + "' value='" + String(slots[i].durationSec) + "' style='width:60px'></td>";
        
        html += "<td>";
        for(int d=1; d<=7; d++) {
            int bitIndex = (d == 7) ? 0 : d; 
            bool isSet = (slots[i].weekDays >> bitIndex) & 1;
            html += "<label class='day-label' style='" + String(isSet ? "background:#cce5ff;border:1px solid #004085;" : "background:#eee;color:#888;") + "'>";
            html += "<input type='checkbox' name='wd_" + base + "_" + String(bitIndex) + "' " + (isSet ? "checked" : "") + " style='margin:0; vertical-align:middle;'>";
            html += dayNames[bitIndex];
            html += "</label>";
            if(d==4) html += "<br>";
        }
        html += "</td></tr>";
    }
    html += "</table>";
    html += "</div>"; 

    html += "<input type='submit' class='btn btn-blue' value='Save Schedule'>";
    html += "</form>";
    html += "<div class='nav'><a href='/'>&laquo; Back to Dashboard</a></div>";
    html += "</body></html>";
    server.send(200, "text/html", html);
}

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
                if (server.hasArg(dParam) && server.arg(dParam) == "on") {
                    slot.weekDays |= (1 << d);
                }
            }
            irrigationUpdateSlot(i, slot);
        }
    }
    irrigationSaveToFlash();
    server.sendHeader("Location", "/schedule", true);
    server.send(302, "text/plain", "Saved");
}

static void handleDiag() {
    if (!checkAuth()) return;
    addNoCacheHeaders();

    String html;
    html.reserve(4096); 
    html += F("<!DOCTYPE html><html><head><meta name='viewport' content='width=device-width, initial-scale=1'>");
    html += COMMON_CSS; 
    html += F("</head><body><h1>Diagnostics</h1>");

    html += F("<div class='card'><h2>Status</h2>");
    html += F("MQTT: ");
    html += (mqttIsConnected() ? F("<b style='color:green'>YES</b>") : F("<b style='color:red'>NO</b>"));
    html += F("<br>State: ");
    html += mqttGetStateString();
    html += F("<br>Last Err: ");
    html += mqttGetLastError();
    html += F("<br>WiFi RSSI: ");
    html += String(wifiGetRssi());
    html += F(" dBm<br>Heap: ");
    html += String(ESP.getFreeHeap());
    html += F("<br>Uptime: ");
    html += String(millis() / 1000);
    html += F(" s</div>");
    
    html += F("<div class='card'><h2>Flow Log (Last 50)</h2><table style='font-size:13px;color:#004488;'>");
    html += logGetFlowEventsHtml(); 
    html += F("</table></div>");

    html += F("<div class='card'><h2>System Events (20)</h2><table style='font-size:13px;'>");
    html += logGetEventsHtml(); 
    html += F("</table></div>");
    
    html += F("<div class='nav'><a href='/'>Home</a></div></body></html>");
    server.send(200, "text/html", html);
}

static void handleRestart() {
    if (!checkAuth()) return;
    server.send(200, "text/plain", "Rebooting...");
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
    String html = String(F("<!DOCTYPE html><html><head><meta name='viewport' content='width=device-width, initial-scale=1'>"));
    html += COMMON_CSS;
    html += F("</head><body><h1>MQTT Setup</h1><div class='card'><form method='POST' action='/mqtt_settings'>");
    html += F("Host:<br><input type='text' name='host' value='");
    html += settingsGetMqttHost();
    html += F("' style='width:100%'><br><br>");
    html += F("Port:<br><input type='number' name='port' value='");
    html += String(settingsGetMqttPort());
    html += F("' style='width:100px'><br><br>");
    html += F("<input type='submit' class='btn btn-blue' value='Save & Reboot'></form></div>");
    html += F("<div class='nav'><a href='/'>Back</a></div></body></html>");
    server.send(200, "text/html", html);
}

static void handleMqttSettingsPost() {
    if (!checkAuth()) return;
    if (server.hasArg("host")) settingsSetMqttHost(server.arg("host"));
    if (server.hasArg("port")) settingsSetMqttPort(server.arg("port").toInt());
    logInfo("MQTT settings updated via Web.");
    server.sendHeader("Location", "/mqtt_config", true);
    server.send(302, "text/plain", "Saved");
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

static void handleApiStatus() {
    if (!checkAuth()) return;
    addNoCacheHeaders();
    String json = buildStatusJson();
    server.send(200, "application/json", json);
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
    server.on("/settings",   HTTP_POST, handleSettingsPost);
    server.on("/mqtt_config",HTTP_GET,  handleMqttConfig);
    server.on("/mqtt_settings", HTTP_POST, handleMqttSettingsPost);
    server.on("/diag",       HTTP_GET,  handleDiag);
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