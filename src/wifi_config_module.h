#pragma once
#include <Arduino.h>

struct NetConfig {
    String ssid;
    String pass;
    String mqttHost;
    int mqttPort; // <--- NEU
};

void netcfgLoad(NetConfig &cfg);
void netcfgSave(const NetConfig &cfg);

// Startet Config-AP und blockiert, bis gespeichert wurde
void wifiConfigPortal();