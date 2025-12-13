#include "time_module.h"
#include "config.h"
#include "logger.h"
#include <WiFi.h>
#include <time.h>

static bool timeSynced = false;

void timeInit() {
    // 1. Zeitzone konfigurieren
    // NTP_TZ_OFFSET_S ist in config.h definiert (7 * 3600 für Thailand)
    configTime(NTP_TZ_OFFSET_S, 0, NTP_SERVER_1);
    
    // FIX FÜR RICHTIGE LOG-ZEITEN (POSIX):
    // "UTC-07:00" bedeutet: Lokalzeit ist UTC + 7h.
    setenv("TZ", "UTC-07:00", 1); 
    tzset();

    logInfo("Time Client init (Target: UTC+7)");
}

void timeLoop() {
    static unsigned long lastCheck = 0;
    if (millis() - lastCheck > 1000) {
        lastCheck = millis();
        
        if (!timeSynced) {
            time_t now;
            time(&now);
            // Wir prüfen, ob wir eine Zeit > 2020 haben (Epoch > 1577836800)
            if (now > 1577836800) {
                timeSynced = true;
                struct tm timeinfo;
                localtime_r(&now, &timeinfo);
                char buf[64];
                sprintf(buf, "NTP Sync Success: %02d.%02d.%04d %02d:%02d:%02d", 
                        timeinfo.tm_mday, timeinfo.tm_mon + 1, timeinfo.tm_year + 1900,
                        timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
                logInfo(buf);
            }
        }
    }
}

String timeGetStr() {
    time_t now;
    time(&now);
    if (now < 1577836800) return "--:--:--";
    
    struct tm timeinfo;
    localtime_r(&now, &timeinfo);
    char buf[16];
    sprintf(buf, "%02d:%02d:%02d", timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
    return String(buf);
}

// === KOMPATIBILITÄTS-FUNKTIONEN (Fix für Linker Errors) ===

// Wird vom Irrigation Module gebraucht
bool timeIsValid() {
    return timeSynced;
}

// Wird vom Irrigation Module gebraucht, um struct tm zu holen
void timeGetLocal(struct tm &out) {
    time_t now;
    time(&now);
    localtime_r(&now, &out);
}

// Wird vom Watchdog Module gesucht (Legacy)
// Wir geben hier FALSE zurück, da der Reboot jetzt zentral von main.cpp gesteuert wird.
bool timeIsDailyResetTime() {
    return false; 
}