// Firmware V0.9.0 – based on V0.8.0, modified in this version.
#include "irrigation_module.h"
#include "config.h"
#include "logger.h"
#include "time_module.h"
#include "valve_module.h"
#include "flow_module.h"    // V0.9 CHANGE: für A5 – Anomalieprüfung
#include <time.h>

static IrrigationProgram prog;
static IrrigationMode mode = IrrigationMode::AUTO;

static bool running = false;
static time_t runStart_t = 0;
static int lastRunDay = -1;  // tm_yday
static time_t lastRunEnd_t = 0; // V0.9 CHANGE: letztes Ende für Logging

// V0.9 CHANGE: Hilfsfunktion für Zeitformat in Logs (A4)
static String fmtTime(const struct tm &tmv) {
    char buf[16];
    snprintf(buf, sizeof(buf), "%02d:%02d:%02d", tmv.tm_hour, tmv.tm_min, tmv.tm_sec);
    return String(buf);
}

void irrigationInit() {
    prog.enabled     = IRR_DEFAULT_ENABLED;
    prog.startHour   = IRR_DEFAULT_START_H;
    prog.startMinute = IRR_DEFAULT_START_M;
    prog.durationSec = IRR_DEFAULT_DURATION_S;
    prog.maxRunSec   = IRR_DEFAULT_MAX_RUN_S;

    running = false;
    lastRunDay = -1;
    lastRunEnd_t = 0;
    mode = IrrigationMode::AUTO;

    logInfo("Irrigation default: enabled=" + String(prog.enabled) +
            " " + String(prog.startHour) + ":" + String(prog.startMinute) +
            " dur=" + String(prog.durationSec) + "s");
}

void irrigationSetMode(IrrigationMode m) {
    mode = m;
    logInfo(String("Irrigation mode set to ") + (m == IrrigationMode::AUTO ? "AUTO" : "MANUAL"));
}

IrrigationMode irrigationGetMode() {
    return mode;
}

IrrigationProgram irrigationGetProgram() {
    return prog;
}

void irrigationSetProgram(const IrrigationProgram &p) {
    prog = p;
    logInfo("Irrigation program updated from config");
}

bool irrigationIsRunning() {
    return running;
}

uint32_t irrigationGetRemainingSec() {
    if (!running) return 0;
    time_t now_t = time(nullptr);
    uint32_t elapsed = (uint32_t)difftime(now_t, runStart_t);
    if (elapsed >= prog.durationSec) return 0;
    return prog.durationSec - elapsed;
}

void irrigationLoop() {
    if (!timeIsValid()) return;
    if (mode != IrrigationMode::AUTO) {
        // V0.9 CHANGE: einfache Anomalieprüfung auch in MANUAL sinnvoll (A5)
        if (valveGetState() == ValveState::CLOSED && flowGetLpm() > 0.2f) {
            logWarn("V0.9: Anomaly: Flow detected while valve CLOSED (MANUAL mode)");
        }
        return;
    }

    time_t t = time(nullptr);
    struct tm now_tm;
    localtime_r(&t, &now_tm);

    // neuer Tag → neuer Programmlauf möglich
    if (lastRunDay != now_tm.tm_yday && !running) {
        // no-op
    }

    if (!running && prog.enabled) {
        if (lastRunDay != now_tm.tm_yday) {
            if (now_tm.tm_hour == prog.startHour &&
                now_tm.tm_min == prog.startMinute) {
                // V0.9 CHANGE (A3): robustere Behandlung, falls Ventil bereits OPEN ist
                if (valveGetState() == ValveState::OPEN) {
                    logWarn("V0.9: AUTO start while valve already OPEN – continuing as AUTO run");
                } else {
                    valveSet(ValveState::OPEN);
                }

                running = true;
                runStart_t = t;
                lastRunDay = now_tm.tm_yday;

                logInfo("Irrigation AUTO start at " + fmtTime(now_tm));
            }
        }
    }

    if (running) {
        uint32_t elapsed = (uint32_t)difftime(t, runStart_t);

        if (elapsed >= prog.durationSec) {
            valveSet(ValveState::CLOSED);
            running = false;
            lastRunEnd_t = t;

            logInfo("Irrigation AUTO stop (program duration reached), elapsed=" +
                    String(elapsed) + "s");
        } else if (elapsed >= prog.maxRunSec) {
            valveSet(ValveState::CLOSED);
            running = false;
            lastRunEnd_t = t;

            logWarn("Irrigation AUTO stop (maxRunSec safety), elapsed=" +
                    String(elapsed) + "s");
        }
    }

    // V0.9 CHANGE (A5): Anomalieprüfung – Flow bei geschlossenem Ventil
    if (!running && valveGetState() == ValveState::CLOSED && flowGetLpm() > 0.2f) {
        logWarn("V0.9: Anomaly: Flow detected while valve CLOSED (AUTO idle)");
    }
}
