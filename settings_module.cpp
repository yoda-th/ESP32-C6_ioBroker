#include "settings_module.h"
#include "config.h"
#include "logger.h"
#include <Preferences.h>

static Preferences prefs;

// Lokale Variablen (Cache) mit Defaults
static int dailyLimitSec = 900; // Default: 15 Minuten
static float batMin = 3.3;      // Default Batterie Min
static float batFactor = 6.47;  // Default Kalibrierung
static int rebootHour = -1;     // Default: Aus (-1)

// MQTT Variablen
static String mqttHost = "192.168.1.10";
static int mqttPort = 1883;

void settingsInit() {
    settingsLoad();
}

void settingsLoad() {
    // "valve-cfg" ist der Namespace im Flash-Speicher
    prefs.begin("valve-cfg", true); // true = read-only mode

    dailyLimitSec = prefs.getInt("limit_sec", 900);
    batMin = prefs.getFloat("bat_min", 3.3);
    
    // Kalibrierfaktor laden (mit Ihrem Default 6.47)
    batFactor = prefs.getFloat("bat_factor", 6.47);
    
    // NEU: Reboot Hour laden (-1 = aus)
    rebootHour = prefs.getInt("reb_h", -1);

    mqttHost = prefs.getString("mqtt_host", "192.168.1.10");
    mqttPort = prefs.getInt("mqtt_port", 1883);

    prefs.end();
    
    logInfo("Settings loaded: Factor=" + String(batFactor) + ", RebootH=" + String(rebootHour));
}

void settingsSave() {
    prefs.begin("valve-cfg", false); // false = read-write

    prefs.putInt("limit_sec", dailyLimitSec);
    prefs.putFloat("bat_min", batMin);
    
    // Kalibrierfaktor speichern
    prefs.putFloat("bat_factor", batFactor);
    
    // NEU: Reboot Hour speichern
    prefs.putInt("reb_h", rebootHour);

    prefs.putString("mqtt_host", mqttHost);
    prefs.putInt("mqtt_port", mqttPort);

    prefs.end();
    logInfo("Settings saved");
}

// === Getter / Setter Implementierung ===

int settingsGetDailyLimitSec() {
    return dailyLimitSec;
}

void settingsSetDailyLimitSec(int s) {
    if (s < 10) s = 10;
    if (s > 7200) s = 7200;
    
    if (s != dailyLimitSec) {
        dailyLimitSec = s;
        settingsSave();
    }
}

float settingsGetBatMin() { return batMin; }

void settingsSetBatMin(float v) {
    if (v != batMin) {
        batMin = v;
        settingsSave();
    }
}

float settingsGetBatFactor() { return batFactor; }

void settingsSetBatFactor(float f) {
    // Sicherheits-Grenzen
    if (f < 1.0) f = 1.0;
    if (f > 10.0) f = 10.0;

    if (f != batFactor) {
        batFactor = f;
        settingsSave();
    }
}

String settingsGetMqttHost() { return mqttHost; }

void settingsSetMqttHost(const String& host) {
    if (mqttHost != host) {
        mqttHost = host;
        settingsSave();
    }
}

int settingsGetMqttPort() { return mqttPort; }

void settingsSetMqttPort(int port) {
    if (mqttPort != port) {
        mqttPort = port;
        settingsSave();
    }
}

// === NEU: Automatischer Reboot ===
int settingsGetRebootHour() { return rebootHour; }

void settingsSetRebootHour(int h) {
    // Validierung: -1 ist erlaubt, sonst 0-23
    if (h < -1) h = -1;
    if (h > 23) h = -1; // Fehlerhafte Werte deaktivieren es lieber
    
    if (h != rebootHour) {
        rebootHour = h;
        settingsSave();
        logInfo("Auto-Reboot set to hour: " + String(rebootHour));
    }
}