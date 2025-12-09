#include "time_module.h"
#include "config.h"
#include "logger.h"
#include "wifi_module.h"

#include <time.h>

static bool time_ok = false;
static unsigned long last_ntp_attempt_ms = 0;
static bool reset_done_today = false;

void timeInit() {
    time_ok = false;
    reset_done_today = false;
    last_ntp_attempt_ms = 0;
}

void timeLoop() {
    if (!wifiIsConnected()) return;

    unsigned long now = millis();
    if (!time_ok && (now - last_ntp_attempt_ms > (NTP_RETRY_S * 1000UL))) {
        last_ntp_attempt_ms = now;
        logInfo("Configuring NTP time...");
        configTime(NTP_TZ_OFFSET_S, 0, NTP_SERVER_1, nullptr, nullptr);
        delay(500);

        time_t t = time(nullptr);
        if (t > 1700000000) { // grobe Schwelle
            time_ok = true;
            struct tm info;
            localtime_r(&t, &info);
            logInfo("Time sync ok");
        } else {
            logWarn("Time not valid yet");
        }
    }

    if (!time_ok) return;

    time_t t = time(nullptr);
    struct tm now_tm;
    localtime_r(&t, &now_tm);

    if (now_tm.tm_hour == 0 && now_tm.tm_min == 0) {
        reset_done_today = false;
    }
}

bool timeIsValid() {
    return time_ok;
}

void timeGetLocal(struct tm &out) {
    time_t t = time(nullptr);
    localtime_r(&t, &out);
}

bool timeIsDailyResetTime() {
    if (!time_ok) return false;

    time_t t = time(nullptr);
    struct tm now_tm;
    localtime_r(&t, &now_tm);

    if (now_tm.tm_hour == DAILY_RESET_H && now_tm.tm_min == DAILY_RESET_M) {
        if (!reset_done_today) {
            reset_done_today = true;
            return true;
        }
    }
    return false;
}
