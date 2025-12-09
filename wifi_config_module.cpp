#include "wifi_config_module.h"
#include "config.h"
#include "logger.h"

#include <WiFi.h>
#include <WebServer.h>
#include <Preferences.h>

static Preferences prefs;
static WebServer cfgServer(80);
static NetConfig currentCfg;

void netcfgLoad(NetConfig &cfg) {
    prefs.begin("netcfg", true); // read-only
    cfg.ssid     = prefs.getString("ssid", WIFI_SSID_DEFAULT);
    cfg.pass     = prefs.getString("pass", WIFI_PASS_DEFAULT);
    cfg.mqttHost = prefs.getString("mqtt", MQTT_HOST_DEFAULT);
    prefs.end();
}

void netcfgSave(const NetConfig &cfg) {
    prefs.begin("netcfg", false);
    prefs.putString("ssid", cfg.ssid);
    prefs.putString("pass", cfg.pass);
    prefs.putString("mqtt", cfg.mqttHost);
    prefs.end();
}

static void handleRoot() {
    String html =
        "<html><body><h1>Valve WiFi / MQTT Config</h1>"
        "<form method='POST' action='/save'>"
        "SSID: <input name='ssid' value='" + currentCfg.ssid + "'><br>"
        "Pass: <input name='pass' type='password' value='" + currentCfg.pass + "'><br>"
        "MQTT Host: <input name='mqtt' value='" + currentCfg.mqttHost + "'><br>"
        "<input type='submit' value='Save & Reboot'>"
        "</form></body></html>";
    cfgServer.send(200, "text/html", html);
}

static void handleSave() {
    if (cfgServer.hasArg("ssid")) currentCfg.ssid = cfgServer.arg("ssid");
    if (cfgServer.hasArg("pass")) currentCfg.pass = cfgServer.arg("pass");
    if (cfgServer.hasArg("mqtt")) currentCfg.mqttHost = cfgServer.arg("mqtt");

    netcfgSave(currentCfg);
    cfgServer.send(200, "text/html", "<html><body><h1>Saved, rebooting...</h1></body></html>");
    logInfo("Config saved, rebooting...");
    delay(1000);
    ESP.restart();
}

void wifiConfigPortal() {
    // Laden aktueller Config als Startwerte
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
    }
}
