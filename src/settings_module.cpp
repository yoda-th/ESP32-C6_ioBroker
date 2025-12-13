#include "settings_module.h"
#include "config.h"
#include "logger.h"
#include <Preferences.h>

static Preferences prefs;

// Lokale Variablen (Cache) mit Defaults
static int dailyLimitSec = 900; // Default: 15 Minuten
static float batMin = 3.3;      // Default Batterie Min
static float batFactor = 6.47;  // Default Bat Kalibrierung
static float flowFactor = 450.0f; // NEU: Default Flow Faktor (Imp/L)
static int rebootHour = -1;     // Default: Aus (-1)

// MQTT Variablen
static String mqttHost = "192.168.1.10";
static int mqttPort = 1883;

void settingsInit() {
    settingsLoad();
}

void settingsLoad() {
    prefs.begin("valve-cfg", true); // true = read-only mode

    dailyLimitSec = prefs.getInt("limit_sec", 900);
    batMin = prefs.getFloat("bat_min", 3.3);
    batFactor = prefs.getFloat("bat_factor", 6.47);
    
    // NEU: Flow Faktor laden
    flowFactor = prefs.getFloat("flow_k", 450.0f);
    
    rebootHour = prefs.getInt("reb_h", -1);
    mqttHost = prefs.getString("mqtt_host", "192.168.1.10");
    mqttPort = prefs.getInt("mqtt_port", 1883);

    prefs.end();
    
    logInfo("Settings loaded: BatFactor=" + String(batFactor) + ", FlowK=" + String(flowFactor));
}

void settingsSave() {
    prefs.begin("valve-cfg", false); // false = read-write

    prefs.putInt("limit_sec", dailyLimitSec);
    prefs.putFloat("bat_min", batMin);
    prefs.putFloat("bat_factor", batFactor);
    
    // NEU: Flow Faktor speichern
    prefs.putFloat("flow_k", flowFactor);
    
    prefs.putInt("reb_h", rebootHour);
    prefs.putString("mqtt_host", mqttHost);
    prefs.putInt("mqtt_port", mqttPort);

    prefs.end();
    logInfo("Settings saved");
}

// ... (Andere Getter/Setter bleiben gleich) ...

int settingsGetDailyLimitSec() { return dailyLimitSec; }
void settingsSetDailyLimitSec(int s) {
    if (s < 10) s = 10; 
    if (s > 7200) s = 7200;
    if (s != dailyLimitSec) { dailyLimitSec = s; settingsSave(); }
}

float settingsGetBatMin() { return batMin; }
void settingsSetBatMin(float v) {
    if (v != batMin) { batMin = v; settingsSave(); }
}

float settingsGetBatFactor() { return batFactor; }
void settingsSetBatFactor(float f) {
    if (f < 1.0) f = 1.0; 
    if (f > 10.0) f = 10.0;
    if (f != batFactor) { batFactor = f; settingsSave(); }
}

// NEU: Flow Faktor Implementation
float settingsGetFlowFactor() { return flowFactor; }
void settingsSetFlowFactor(float f) {
    if (f < 10.0) f = 10.0;   // Plausibilität
    if (f > 2000.0) f = 2000.0;
    if (f != flowFactor) {
        flowFactor = f;
        settingsSave();
    }
}

String settingsGetMqttHost() { return mqttHost; }
void settingsSetMqttHost(const String& host) {
    if (mqttHost != host) { mqttHost = host; settingsSave(); }
}

int settingsGetMqttPort() { return mqttPort; }
void settingsSetMqttPort(int port) {
    if (mqttPort != port) { mqttPort = port; settingsSave(); }
}

int settingsGetRebootHour() { return rebootHour; }
void settingsSetRebootHour(int h) {
    if (h < -1) h = -1;
    if (h > 23) h = -1;
    if (h != rebootHour) { rebootHour = h; settingsSave(); logInfo("Auto-Reboot set to hour: " + String(rebootHour)); }
}