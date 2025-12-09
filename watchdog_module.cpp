#include "watchdog_module.h"
#include "time_module.h"
#include "logger.h"
#include "valve_module.h"
#include "irrigation_module.h"
#include <esp_task_wdt.h>

// 10 Sekunden Timeout
static const int WDT_TIMEOUT_MS = 10000;

void watchdogInit() {
    // --- FIX FÜR ESP32-C6 (ESP-IDF v5 API) ---
    // Statt esp_task_wdt_init(10, true) nutzen wir jetzt eine Config-Struktur
    esp_task_wdt_config_t twdt_config = {
        .timeout_ms = WDT_TIMEOUT_MS,
        .idle_core_mask = (1 << 0), // Bitmaske für Core 0 (C6 hat nur Core 0)
        .trigger_panic = true       // Reboot bei Hänger
    };
    
    esp_task_wdt_init(&twdt_config);
    esp_task_wdt_add(NULL); // Fügt den aktuellen Loop-Task zur Überwachung hinzu
    
    logInfo("Watchdog initialized (ESP32-C6 API)");
}

void watchdogFeed() {
    esp_task_wdt_reset();
}

void watchdogLoop() {
    // Täglicher Neustart-Logik
    if (timeIsDailyResetTime()) {
        if (irrigationIsRunning()) {
            logWarn("V0.9: Daily reset postponed – irrigation running");
            return;
        }

        logWarn("Daily reset time reached -> safe close valve & reboot");
        valveSafeBeforeUpdate();
        delay(500);
        ESP.restart();
    }
}