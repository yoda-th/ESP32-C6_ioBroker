#include "wifi_config_module.h"
#include "config.h"
#include "logger.h"
#include "settings_module.h" 

#include <WiFi.h>
#include <WebServer.h>
#include <Preferences.h>

static Preferences prefs;
static WebServer cfgServer(80);
static NetConfig currentCfg;

void netcfgLoad(NetConfig &cfg) {
    // 1. WLAN (aus netcfg)
    prefs.begin("netcfg", true); 
    cfg.ssid = prefs.getString("ssid", WIFI_SSID_DEFAULT);
    cfg.pass = prefs.getString("pass", WIFI_PASS_DEFAULT);
    prefs.end();

    // 2. MQTT (aus settings / valve-cfg)
    cfg.mqttHost = settingsGetMqttHost();
    cfg.mqttPort = settingsGetMqttPort(); // <--- Port laden
}

void netcfgSave(const NetConfig &cfg) {
    // 1. WLAN speichern
    prefs.begin("netcfg", false); 
    prefs.putString("ssid", cfg.ssid);
    prefs.putString("pass", cfg.pass);
    prefs.end();

    // 2. MQTT zentral speichern
    settingsSetMqttHost(cfg.mqttHost);
    settingsSetMqttPort(cfg.mqttPort); // <--- Port speichern
}

static void handleRoot() {
    String html = "<html><head><title>Setup</title>";
    html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
    html += "<style>body{font-family:sans-serif;padding:20px;} input{padding:10px;width:100%;margin-bottom:10px;box-sizing:border-box;} button{padding:15px;width:100%;background:#007bff;color:white;border:none;font-size:16px;}</style>";
    html += "</head><body><h1>Valve Setup</h1>";
    
    html += "<form method='POST' action='/save'>";
    
    html += "<h3>WiFi</h3>";
    html += "SSID:<br><input name='ssid' value='" + currentCfg.ssid + "'><br>";
    html += "Pass:<br><input name='pass' type='password' value='" + currentCfg.pass + "'><br>";
    
    html += "<h3>MQTT</h3>";
    html += "Host:<br><input name='mqtt' value='" + currentCfg.mqttHost + "'><br>";
    html += "Port:<br><input name='port' type='number' value='" + String(currentCfg.mqttPort) + "'><br><br>";
    
    html += "<button type='submit'>Save & Reboot</button>";
    html += "</form></body></html>";
    
    cfgServer.send(200, "text/html", html);
}

static void handleSave() {
    if (cfgServer.hasArg("ssid")) currentCfg.ssid = cfgServer.arg("ssid");
    if (cfgServer.hasArg("pass")) currentCfg.pass = cfgServer.arg("pass");
    
    if (cfgServer.hasArg("mqtt")) currentCfg.mqttHost = cfgServer.arg("mqtt");
    if (cfgServer.hasArg("port")) currentCfg.mqttPort = cfgServer.arg("port").toInt();

    netcfgSave(currentCfg);

    String html = "<html><head><meta name='viewport' content='width=device-width, initial-scale=1'></head>";
    html += "<body><h1>Saved!</h1><p>Rebooting...</p></body></html>";
    cfgServer.send(200, "text/html", html);
    
    logInfo("Config saved via Portal. Rebooting...");
    delay(1000);
    ESP.restart();
}

void wifiConfigPortal() {
    netcfgLoad(currentCfg);

    logWarn("Starting WiFi CONFIG portal (PIN2 LOW)");
    
    WiFi.mode(WIFI_AP);
    String apName = String("ValveConfig-") + String((uint32_t)ESP.getEfuseMac(), HEX);
    WiFi.softAP(apName.c_str(), "config123");

    IPAddress ip = WiFi.softAPIP();
    logInfo("AP: " + apName + " IP: " + ip.toString());

    cfgServer.on("/", HTTP_GET, handleRoot);
    cfgServer.on("/save", HTTP_POST, handleSave);
    cfgServer.begin();

    while (true) {
        cfgServer.handleClient();
        delay(10);
        // Blinken
        static unsigned long lastBlink = 0;
        if (millis() - lastBlink > 300) {
            lastBlink = millis();
            digitalWrite(PIN_STATUS_LED, !digitalRead(PIN_STATUS_LED));
        }
    }
}