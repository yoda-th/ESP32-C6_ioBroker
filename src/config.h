#pragma once

// ==========================================================
// FIRMWARE KONFIGURATION
// ==========================================================
#define FW_VERSION      "1.2.8" 
#define DEVICE_NAME     "ESP-Valve-C6-01"
#define MQTT_CLIENT_ID  "esp-valve-c6-01"

// Payload Definitionen
#define MQTT_PAYLOAD_ONLINE  "Online"
#define MQTT_PAYLOAD_OFFLINE "Offline"

// ==========================================================
// PINS (Ihre Hardware)
// ==========================================================
#define PIN_RELAY       4       
#define PIN_FLOW        5       
#define PIN_BATTERY_ADC 1       
#define PIN_STATUS_LED  8       
#define PIN_CFG_BUTTON  2       

#define PIN_VALVE_A     PIN_RELAY 

// ==========================================================
// WIFI & MQTT KONFIGURATION
// ==========================================================
#define WIFI_SSID_DEFAULT   "DEIN_WLAN"
#define WIFI_PASS_DEFAULT   "DEIN_PASS"
#define MQTT_HOST_DEFAULT   "192.168.1.104"
#define MQTT_PORT           1883
#define MQTT_BASE_TOPIC     "garden/valve1"

// ==========================================================
// MQTT TOPICS (NUR DEKLARATION!)
// ==========================================================
// Das 'extern' sagt: "Die Variable liegt woanders (in config.cpp)"
extern const char* TOPIC_STATE;
extern const char* TOPIC_TELE;
extern const char* TOPIC_CMD;
extern const char* TOPIC_CFG;
extern const char* TOPIC_DIAG;
extern const char* TOPIC_PROG;
extern const char* TOPIC_LWT;
extern const char* TOPIC_HISTORY;
extern const char* TOPIC_USAGE;
extern const char* TOPIC_LIMIT;
extern const char* TOPIC_EVENT;
extern const char* TOPIC_LOG;

// (Optional für Code-Kompatibilität, falls noch X-Macro Reste da sind)
#define MQTT_TOPIC_GENERATOR(X) 

// ==========================================================
// EINSTELLUNGEN
// ==========================================================
#define OTA_USER        "otauser"
#define OTA_PASS        "superSecret123"

#define NTP_SERVER_1    "pool.ntp.org"
#define NTP_TZ_OFFSET_S (7 * 3600) // UTC+7
#define NTP_RETRY_S     60

#define FLOW_K_FACTOR 7.5f 
#define BAT_R1 100.0f
#define BAT_R2 100.0f

#define IRR_DEFAULT_ENABLED     true
#define IRR_DEFAULT_START_H     6
#define IRR_DEFAULT_START_M     0
#define IRR_DEFAULT_DURATION_S  600
#define IRR_DEFAULT_MAX_RUN_S   3600