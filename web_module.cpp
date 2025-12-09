// Firmware V0.9.0 – based on V0.8.0, modified in this version.
#include "web_module.h"
#include "config.h"
#include "logger.h"
#include "wifi_module.h"
#include "valve_module.h"
#include "flow_module.h"
#include "battery_module.h"
#include "irrigation_module.h"   // V0.9 CHANGE: Irrigation-Infos in /api/status

#include <WebServer.h>
#include <Update.h>

static WebServer server(80);

// V0.9 CHANGE: kleine Helper für No-Cache-Header
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

static String buildStatusJson() {
    IrrigationProgram p = irrigationGetProgram(); // V0.9 CHANGE

    String json = "{";
    json += "\"fw\":\"" + String(FW_VERSION) + "\",";
    json += "\"device\":\"" + String(DEVICE_NAME) + "\",";
    json += "\"wifi_ip\":\"" + wifiGetIp() + "\",";
    json += "\"wifi_rssi\":" + String(wifiGetRssi()) + ",";
    json += "\"valve\":\"" + String(valveGetState() == ValveState::OPEN ? "OPEN" : "CLOSED") + "\",";
    json += "\"flow_lpm\":" + String(flowGetLpm(), 2) + ",";
    json += "\"flow_total_l\":" + String(flowGetTotalLiters(), 2) + ",";
    json += "\"battery_v\":" + String(batteryGetVoltage(), 2) + ",";
    // V0.9 CHANGE: Irrigation-Infos ergänzen
    json += "\"irr_mode\":\"" + String(irrigationGetMode() == IrrigationMode::AUTO ? "AUTO" : "MANUAL") + "\",";
    json += "\"irr_running\":" + String(irrigationIsRunning() ? "true" : "false") + ",";
    json += "\"irr_start_h\":" + String(p.startHour) + ",";
    json += "\"irr_start_m\":" + String(p.startMinute) + ",";
    json += "\"irr_dur_s\":" + String(p.durationSec);
    json += "}";
    return json;
}

static void handleRoot() {
    if (!checkAuth()) return;
    addNoCacheHeaders();              // V0.9 CHANGE
    String html = "<html><head><title>";
    html += DEVICE_NAME;
    html += "</title>";
    html += "<meta http-equiv='Cache-Control' content='no-store'/>"; // V0.9 CHANGE
    html += "</head><body>";
    html += "<h1>";
    html += DEVICE_NAME;
    html += " (" FW_VERSION ")</h1>";
    html += "<p>IP: " + wifiGetIp() + "</p>";
    html += "<p>Valve: ";
    html += (valveGetState() == ValveState::OPEN ? "OPEN" : "CLOSED");
    html += "</p>";
    html += "<p>Flow: ";
    html += String(flowGetLpm(), 2);
    html += " L/min, Total: ";
    html += String(flowGetTotalLiters(), 2);
    html += " L</p>";
    html += "<p>Battery: ";
    html += String(batteryGetVoltage(), 2);
    html += " V</p>";

    IrrigationProgram p = irrigationGetProgram();
    html += "<p>Irrigation mode: ";
    html += (irrigationGetMode() == IrrigationMode::AUTO ? "AUTO" : "MANUAL");
    html += ", start: ";
    html += String(p.startHour) + ":" + String(p.startMinute);
    html += ", dur: ";
    html += String(p.durationSec);
    html += "s</p>";

    html += "<form method='POST' action='/valve'>";
    html += "<button name='state' value='open'>OPEN</button>";
    html += "<button name='state' value='close'>CLOSE</button>";
    html += "</form>";
    html += "<p><a href='/update'>Firmware Update</a></p>";
    html += "</body></html>";
    server.send(200, "text/html", html);
}

static void handleValvePost() {
    if (!checkAuth()) return;
    if (server.hasArg("state")) {
        String s = server.arg("state");
        if (s == "open") {
            irrigationSetMode(IrrigationMode::MANUAL); // V0.9 CHANGE
            valveSet(ValveState::OPEN);
        } else {
            irrigationSetMode(IrrigationMode::MANUAL);
            valveSet(ValveState::CLOSED);
        }
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
    String html =
        "<html><body><h1>OTA Update (" FW_VERSION ")</h1>"
        "<form method='POST' action='/update' enctype='multipart/form-data'>"
        "<input type='file' name='update' accept='.bin'>"
        "<input type='submit' value='Upload'>"
        "</form></body></html>";
    server.send(200, "text/html", html);
}

static void handleUpdatePost() {
    if (!checkAuth()) return;

    HTTPUpload &upload = server.upload();
    if (upload.status == UPLOAD_FILE_START) {
        logWarn("OTA start: " + upload.filename);
        valveSafeBeforeUpdate();
        if (!Update.begin()) {
            Update.printError(Serial);
        }
    } else if (upload.status == UPLOAD_FILE_WRITE) {
        if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
            Update.printError(Serial);
        }
    } else if (upload.status == UPLOAD_FILE_END) {
        if (Update.end(true)) {
            logInfo("OTA success, size=" + String(upload.totalSize));
            server.send(200, "text/html",
                        "<html><body><h1>Update OK (" FW_VERSION ")</h1><p>Reboot...</p></body></html>");
            valveSafeAfterUpdate();
            delay(1000);
            ESP.restart();
        } else {
            Update.printError(Serial);
            server.send(500, "text/plain", "Update failed");
        }
    } else if (upload.status == UPLOAD_FILE_ABORTED) {
        Update.end();
        logError("OTA aborted");
    }
}

void webInit() {
    server.on("/", HTTP_GET, handleRoot);
    server.on("/valve", HTTP_POST, handleValvePost);
    server.on("/api/status", HTTP_GET, handleApiStatus);
    server.on("/update", HTTP_GET, handleUpdateGet);
    server.on("/update", HTTP_POST, [](){}, handleUpdatePost);

    server.begin();
    logInfo("Web/OTA server started on port 80");
}

void webLoop() {
    server.handleClient();
}
