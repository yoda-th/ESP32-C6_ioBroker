/****************************************************
 * web_module.cpp
 * HTTP-Status + OTA-Update für ESP-Valve-C6
 * 
 * Firmware-Linie: V0.9.x
 * Stand: 2025-12-09 – ergänzt um Anzeige der MQTT-Topics
 ****************************************************/

#include "web_module.h"
#include "config.h"
#include "logger.h"
#include "wifi_module.h"
#include "valve_module.h"
#include "flow_module.h"
#include "battery_module.h"
#include "irrigation_module.h"
//#include "mqtt_topics.h"

#include <WebServer.h>
#include <Update.h>

static WebServer server(80);

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

// JSON für /api/status
static String buildStatusJson() {
    IrrigationProgram p = irrigationGetProgram();

    String json = "{";
    json += "\"fw\":\"" + String(FW_VERSION) + "\",";
    json += "\"device\":\"" + String(DEVICE_NAME) + "\",";
    json += "\"wifi_ip\":\"" + wifiGetIp() + "\",";
    json += "\"wifi_rssi\":" + String(wifiGetRssi()) + ",";
    json += "\"valve\":\"" + String(valveGetState() == ValveState::OPEN ? "OPEN" : "CLOSED") + "\",";
    json += "\"flow_lpm\":" + String(flowGetLpm(), 2) + ",";
    json += "\"flow_total_l\":" + String(flowGetTotalLiters(), 2) + ",";
    json += "\"battery_v\":" + String(batteryGetVoltage(), 2) + ",";
// 0.9.9 NEU:
    json += "\"last_diag\":\"" + logGetLastDiag() + "\",";

    json += "\"irr_mode\":\"" + String(irrigationGetMode() == IrrigationMode::AUTO ? "AUTO" : "MANUAL") + "\",";
    json += "\"irr_running\":" + String(irrigationIsRunning() ? "true" : "false") + ",";
    json += "\"irr_start_h\":" + String(p.startHour) + ",";
    json += "\"irr_start_m\":" + String(p.startMinute) + ",";
    json += "\"irr_dur_s\":" + String(p.durationSec) + ",";

// Hilfs-Makro: Erzeugt JSON Key-Value
    #define GENERATE_JSON_ITEM(name, path, label) \
        json += "\"mqtt_" #name "\":\"" + String(TOPIC_##name) + "\",";

    // Generieren
    MQTT_TOPIC_GENERATOR(GENERATE_JSON_ITEM)

    json += "\"meta\":\"auto_generated\"";
    json += "}";
    return json;
}


// Hauptseite mit Status + MQTT-Topics
static void handleRoot() {
    if (!checkAuth()) return;
    addNoCacheHeaders();

    IrrigationProgram p = irrigationGetProgram();

    String html = "<html><head><title>";
    html += DEVICE_NAME;
    html += "</title>";
    html += "<meta http-equiv='Cache-Control' content='no-store'/>";
    html += "<style>body{font-family:sans-serif;}pre{background:#eee;padding:8px;}</style>";
    html += "</head><body>";

    html += "<h1>";
    html += DEVICE_NAME;
    html += " (" FW_VERSION ")</h1>";

    html += "<h2>Status</h2>";
    html += "<p>IP: " + wifiGetIp() + "</p>";
    html += "<p>WiFi RSSI: " + String(wifiGetRssi()) + " dBm</p>";

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

// 0.9.9 === NEU: DIAGNOSE ANZEIGE MIT BUTTON ===
    String diag = logGetLastDiag();
    html += "<p>Diagnostics: ";
    
    if (diag.startsWith("ALARM")) {
        // ROTE SCHRIFT + BUTTON
        html += "<b style='color:red;'>" + diag + "</b>";
        html += "</p>"; // Absatz schließen
        
        // Der Button zum Bestätigen
        html += "<form method='POST' action='/clear_diag'>";
        html += "<button style='background:#ffcccc; border:1px solid red; cursor:pointer;'>";
        html += "&#10004; Alarm best&auml;tigen"; // Haken-Symbol + Text
        html += "</button></form>";
        
    } else {
        // GRÜNE SCHRIFT (Kein Button nötig)
        html += "<span style='color:green;'>" + diag + "</span>";
        html += "</p>";
    }
    // ===================================

    html += "<p>Irrigation mode: ";
    html += "<p>Irrigation mode: ";
    html += (irrigationGetMode() == IrrigationMode::AUTO ? "AUTO" : "MANUAL");
    html += ", start: ";
    html += String(p.startHour) + ":" + String(p.startMinute);
    html += ", dur: ";
    html += String(p.durationSec);
    html += "s</p>";

    html += "<h2>Valve Control</h2>";
    html += "<form method='POST' action='/valve'>";
    html += "<button name='state' value='open'>OPEN</button>";
    html += "<button name='state' value='close'>CLOSE</button>";
    html += "</form>";

html += "<h2>MQTT Configuration</h2>";
    html += "<p>Base: <b>" MQTT_BASE_TOPIC "</b></p>";
    
    html += "<table border='1' cellpadding='5' cellspacing='0' style='border-collapse:collapse; width:100%;'>";
    html += "<tr style='background:#eee;'><th>Name</th><th>Topic Path</th></tr>";

    // === DIE AUTOMATISCHE TABELLE ===
    // Wir nutzen den Generator aus config.h erneut!
    
    // Hilfs-Makro: Erzeugt eine Tabellenzeile
    #define GENERATE_HTML_ROW(name, path, label) \
        html += "<tr>"; \
        html += "<td><b>" label "</b></td>"; \
        html += "<td>" + String(TOPIC_##name) + "</td>"; \
        html += "</tr>";

    // Hier wird der Code für alle Zeilen "ausgepackt"
    MQTT_TOPIC_GENERATOR(GENERATE_HTML_ROW)

    html += "</table>";


    html += "<h2>Tools</h2>";
    html += "<p><a href='/update'>Firmware Update</a></p>";
    html += "<p><a href='/api/status'>API Status (JSON)</a></p>";

    html += "</body></html>";
    server.send(200, "text/html", html);
}

static void handleValvePost() {
    if (!checkAuth()) return;
    if (server.hasArg("state")) {
        String s = server.arg("state");
        irrigationSetMode(IrrigationMode::MANUAL);
        if (s == "open") {
            valveSet(ValveState::OPEN);
        } else {
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
// NEU: Alarm zurücksetzen
static void handleClearDiagPost() {
    if (!checkAuth()) return;
    
    // Wir überschreiben den Alarm-Text mit "OK"
    logSetLastDiag("OK");
    
    // Zurück zur Hauptseite
    server.sendHeader("Location", "/", true);
    server.send(302, "text/plain", "Redirecting");
}
void webInit() {
    server.on("/",           HTTP_GET,  handleRoot);
    server.on("/valve",      HTTP_POST, handleValvePost);

// === NEU: ===
    server.on("/clear_diag", HTTP_POST, handleClearDiagPost); 
// ============

    server.on("/api/status", HTTP_GET,  handleApiStatus);
    server.on("/update",     HTTP_GET,  handleUpdateGet);
    server.on("/update",     HTTP_POST, [](){}, handleUpdatePost);

    server.begin();
    logInfo("Web/OTA server started on port 80");
}

void webLoop() {
    server.handleClient();
}