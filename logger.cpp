#include "logger.h"
#include "config.h"
#include <deque> // Für die Listen
#include <time.h>

// Speicher für die letzte Meldung
static String lastDiagMessage = "OK"; 

// 1. SYSTEM LOG (Fehler, WLAN, Reboot) - Max 20
static std::deque<String> eventLog; 
const size_t MAX_LOG_EVENTS = 20;   

// 2. FLOW LOG (Ventil auf/zu, Liter) - Max 50
static std::deque<String> flowLog; 
const size_t MAX_FLOW_EVENTS = 50;   

// Hilfsfunktion: Zeitstempel (HH:MM:SS)
String getLogTimeStr() {
    time_t now;
    time(&now);
    // Wenn NTP noch nicht da (Zeit < 2020)
    if (now < 1577836800) {
        return "[" + String(millis()/1000) + "s]";
    }
    struct tm timeinfo;
    localtime_r(&now, &timeinfo);
    char buf[16];
    sprintf(buf, "%02d:%02d:%02d", timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
    return String(buf);
}

// === SYSTEM LOG LOGIK ===
void logEvent(String type, String msg) {
    String entry = getLogTimeStr() + " <b>[" + type + "]</b> " + msg;
    eventLog.push_front(entry); 
    if (eventLog.size() > MAX_LOG_EVENTS) eventLog.pop_back(); 
}

String logGetEventsHtml() {
    String html = "";
    if (eventLog.empty()) return "<tr><td>No events yet</td></tr>";
    for (const auto& line : eventLog) {
        html += "<tr><td style='font-size:14px; padding:4px; border-bottom:1px solid #eee;'>" + line + "</td></tr>";
    }
    return html;
}

// === NEU: FLOW LOG LOGIK (Größe 50) ===
void logFlowEvent(String msg) {
    // Einfaches Format: "HH:MM:SS Nachricht"
    String entry = getLogTimeStr() + " " + msg;
    
    flowLog.push_front(entry);
    if (flowLog.size() > MAX_FLOW_EVENTS) {
        flowLog.pop_back();
    }
    // Optional auch auf Serial ausgeben
    Serial.println("[FLOW-LOG] " + msg);
}

String logGetFlowEventsHtml() {
    String html = "";
    if (flowLog.empty()) return "<tr><td>No flow events yet</td></tr>";
    for (const auto& line : flowLog) {
        html += "<tr><td style='font-size:13px; padding:2px; border-bottom:1px solid #f0f0f0; color:#0055aa;'>" + line + "</td></tr>";
    }
    return html;
}

// === STANDARD LOGGER ===
void logInit() {
    Serial.begin(115200);
    delay(1000);
    Serial.println();
    Serial.println("=== Logger started ===");
    logEvent("SYSTEM", "Bootup " FW_VERSION);
}

void logInfo(const String &msg) {
    Serial.println("[INFO] " + msg);
}

void logWarn(const String &msg) {
    Serial.println("[WARN] " + msg);
    logEvent("WARN", msg);
}

void logError(const String &msg) {
    Serial.println("[ERROR] " + msg);
    logEvent("ERROR", msg);
}

void logDiag(const String &msg) {
    Serial.println("[DIAG] " + msg);
}

void logSetLastDiag(String s) {
    if (lastDiagMessage != s) {
        lastDiagMessage = s;
        logEvent("STATUS", s);
    }
}

String logGetLastDiag() {
    return lastDiagMessage;
}