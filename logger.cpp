// 09.9.9
#include "logger.h"
#include <Arduino.h>

// Speicher für die letzte Meldung (Default: "OK")
static String lastDiagMessage = "OK"; 

void logInit() {
    Serial.begin(115200);
    delay(1000);
    Serial.println();
    Serial.println("=== Logger started ===");
}

// Hier nutzen wir jetzt "const String &msg", genau wie im Header!
void logInfo(const String &msg) {
    Serial.println("[INFO] " + msg);
}

void logWarn(const String &msg) {
    Serial.println("[WARN] " + msg);
}

void logError(const String &msg) {
    Serial.println("[ERROR] " + msg);
}

void logDiag(const String &msg) {
    Serial.println("[DIAG] " + msg);
}

// In Ihrem Header war logSetLastDiag als "String s" definiert (by value)
// Daher lassen wir das hier so, damit es matcht.
void logSetLastDiag(String s) {
    lastDiagMessage = s;
}

String logGetLastDiag() {
    return lastDiagMessage;
}