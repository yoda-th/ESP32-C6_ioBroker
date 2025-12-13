#pragma once

// ==========================================================
// FIRMWARE KONFIGURATION
// ==========================================================
#define FW_VERSION      "1.2.7" 
#define DEVICE_NAME     "ESP-Valve-C6-01"
#define MQTT_CLIENT_ID  "esp-valve-c6-01"

// Payload Definitionen
#define MQTT_PAYLOAD_ONLINE  "Online"
#define MQTT_PAYLOAD_OFFLINE "Offline"

// ==========================================================
// PINS (Aus deiner V1.0.2 - Bestätigt)
// ==========================================================
#define PIN_RELAY       4       // Ventil-Relais
#define PIN_FLOW        5       // Flow-Sensor
#define PIN_BATTERY_ADC 1       // Batteriespannung
#define PIN_STATUS_LED  8       // Status-LED
#define PIN_CFG_BUTTON  2       // Config Button

// Alias für Kompatibilität
#define PIN_VALVE_A     PIN_RELAY 

// ==========================================================
// WIFI & MQTT
// ==========================================================
#define WIFI_SSID_DEFAULT   "DEIN_WLAN"
#define WIFI_PASS_DEFAULT   "DEIN_PASS"
#define MQTT_HOST_DEFAULT   "192.168.1.104"
#define MQTT_PORT           1883
#define MQTT_BASE_TOPIC     "garden/valve1"

// ==========================================================
// TOPICS (Deine Liste + LOG für Web)
// ==========================================================
#define MQTT_TOPIC_GENERATOR(X) \
    X(STATE,   "/stat",   "Status (JSON)") \
    X(TELE,    "/tele",   "Telemetrie") \
    X(CMD,     "/cmnd",   "Command") \
    X(CFG,     "/cfg",    "Config") \
    X(DIAG,    "/diag",   "Diagnose") \
    X(PROG,    "/prog",   "Program") \
    X(LWT,     "/lwt",    "LWT") \
    X(HISTORY, "/hist",   "History (Puffer)") \
    X(USAGE,   "/usage",  "Täglicher Verbrauch") \
    X(LIMIT,   "/limit",  "Tägliches Limit") \
    X(EVENT,   "/event",  "System Events (JSON)") \
    X(LOG,     "/log",    "Live Log (Text)") // Neu für Web

#define X_GEN_TOPIC(name, path, desc) const char* TOPIC_##name = MQTT_BASE_TOPIC path;
MQTT_TOPIC_GENERATOR(X_GEN_TOPIC)

// ==========================================================
// EINSTELLUNGEN
// ==========================================================
#define OTA_USER        "otauser"
#define OTA_PASS        "superSecret123"

#define NTP_SERVER_1    "pool.ntp.org"
#define NTP_TZ_OFFSET_S (7 * 3600) // UTC+7
#define NTP_RETRY_S     60

// Kalibrierung (aus deinem Code)
#define FLOW_K_FACTOR 7.5f 
#define BAT_R1 100.0f
#define BAT_R2 100.0f

// Defaults
#define IRR_DEFAULT_ENABLED     true
#define IRR_DEFAULT_START_H     6
#define IRR_DEFAULT_START_M     0
#define IRR_DEFAULT_DURATION_S  600
#define IRR_DEFAULT_MAX_RUN_S   3600