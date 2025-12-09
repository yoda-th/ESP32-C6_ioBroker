#include "logger.h"

void logInit() {
    Serial.begin(115200);
    delay(500);
    Serial.println();
    Serial.println("=== Logger started ===");
}

void logInfo(const String &msg) {
    Serial.print("[INFO] ");
    Serial.println(msg);
}

void logWarn(const String &msg) {
    Serial.print("[WARN] ");
    Serial.println(msg);
}

void logError(const String &msg) {
    Serial.print("[ERROR] ");
    Serial.println(msg);
}

void logDiag(const String &msg) {
    Serial.print("[DIAG] ");
    Serial.println(msg);
}
