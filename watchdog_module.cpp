// Firmware V0.9.0 – based on V0.8.0, modified in this version.
#include "watchdog_module.h"
#include "time_module.h"
#include "logger.h"
#include "valve_module.h"
#include "irrigation_module.h"   // V0.9 CHANGE: um laufende Bewässerung zu berücksichtigen

#include <esp_task_wdt.h>

static const int WDT_TIMEOUT_S = 10;   // 10s ohne Feed -> Reset

void watchdogInit() {
    esp_task_wdt_init(WDT_TIMEOUT_S, true);
    esp_task_wdt_add(NULL);
    logInfo("Watchdog initialized");
}

void watchdogFeed() {
    esp_task_wdt_reset();
}

void watchdogLoop() {
    if (timeIsDailyResetTime()) {
        // V0.9 CHANGE: Kein täglicher Reset, wenn Bewässerung läuft
        if (irrigationIsRunning()) {
            logWarn("V0.9: Daily reset time reached but irrigation running – postponing reset");
            return;
        }

        logWarn("Daily reset time reached -> safe close valve & reboot");
        valveSafeBeforeUpdate();
        delay(500);
        ESP.restart();
    }
}
