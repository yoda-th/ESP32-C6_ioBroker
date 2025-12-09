// Firmware V0.9.0 – based on V0.8.0, modified in this version.
//V0.9.7 MQTT Topics angepasst auf bestehende ioBroker ID
// web_module.cpp — Updated version with dynamic MQTT topic display
#pragma once

// ========= Firmware / Device =========
#define FW_VERSION      "0.9.9-C6"
#define DEVICE_NAME     "ESP-Valve-C6-01"
#define MQTT_CLIENT_ID  "esp-valve-c6-01"

// ========= Pins =========
#define PIN_RELAY       4       // Ventil-Relais
#define PIN_FLOW        5       // Flow-Sensor
#define PIN_BATTERY_ADC 1       // Batteriespannung JB:von 2 auf 1 geaendert
#define PIN_STATUS_LED  8       // Status-LED
#define PIN_CFG_BUTTON  2       // LOW = Config-Modus bei Boot - JB von 9 auf 2 geaendert

// ========= WiFi Default (Fallback, wenn keine Config im NVS) =========
#define WIFI_SSID_DEFAULT   "DEIN_WLAN"
#define WIFI_PASS_DEFAULT   "DEIN_PASS"

// ========= MQTT Default =========
#define MQTT_HOST_DEFAULT   "192.168.1.61"  // Raspi / Mosquitto / ioBroker
#define MQTT_PORT           1883

// ========= MQTT Konfiguration =========
#define MQTT_BASE_TOPIC "garden/valve1"

// ==========================================================
// DIE MASTER-LISTE (X-Macro)
// Hier definieren Sie alle Topics EINMALIG.
// Syntax: X(Name_im_Code,  Endung_im_MQTT,  Anzeige_Webseite)
// ==========================================================
#define MQTT_TOPIC_GENERATOR(X) \
    X(STATE, "/stat", "Status")   \
    X(CMD,   "/cmnd", "Command")  \
    X(CFG,   "/cfg",  "Config")   \
    X(DIAG,  "/diag", "Diagnose") \
    X(PROG,  "/prog", "Program")  \
    X(LWT,   "/lwt",  "LWT")      \
    X(WETTER, "/wetter", "Wetterbericht")

// --- AB HIER PASSIERT DIE MAGIE AUTOMATISCH (NICHT ÄNDERN) ---

// 1. Automatische Erzeugung der Variablen (z.B. TOPIC_CMD)
// Der Compiler baut daraus: static const char* TOPIC_CMD = "garden/valve1/cmnd";
#define GENERATE_VAR(name, path, label)  static const char* TOPIC_##name = MQTT_BASE_TOPIC path;

MQTT_TOPIC_GENERATOR(GENERATE_VAR)

// ========= OTA Web-Login =========
#define OTA_USER        "otauser"
#define OTA_PASS        "superSecret123"

// ========= Zeit / NTP (Thailand, UTC+7, keine DST) =========
#define NTP_SERVER_1    "pool.ntp.org"
#define NTP_TZ_OFFSET_S (7 * 3600)   // UTC+7
#define NTP_RETRY_S     60           // alle 60s neu versuchen, bis Zeit da ist
#define DAILY_RESET_H   4            // 04:00 lokaler Zeit
#define DAILY_RESET_M   0

// ========= Default-Bewässerungsprogramm (kann via MQTT überschrieben werden) =========
// 1x täglich um 06:00 für 10 Minuten
#define IRR_DEFAULT_ENABLED   true
#define IRR_DEFAULT_START_H   6
#define IRR_DEFAULT_START_M   0
#define IRR_DEFAULT_DURATION_S  (10 * 60)  // 10 Minuten
#define IRR_DEFAULT_MAX_RUN_S   (60 * 60)  // Safety: max. 1 Stunde
