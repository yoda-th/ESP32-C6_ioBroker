// Firmware V0.9.0 – based on V0.8.0, modified in this version.
#pragma once

// ========= Firmware / Device =========
// V0.9 CHANGE: Version von 0.8.0 auf 0.9.0 erhöht
#define FW_VERSION      "0.9.0"
#define DEVICE_NAME     "ESP-Valve-C6-01"
#define MQTT_CLIENT_ID  "esp-valve-c6-01"

// ========= Pins =========
#define PIN_RELAY       4       // Ventil-Relais
#define PIN_FLOW        5       // Flow-Sensor
#define PIN_BATTERY_ADC 2       // Batteriespannung
#define PIN_STATUS_LED  8       // Status-LED
#define PIN_CFG_BUTTON  9       // LOW = Config-Modus bei Boot

// ========= WiFi Default =========
#define WIFI_SSID_DEFAULT   "DEIN_WLAN"  //SSID and password can be placed here, but it is not secure here. Therefore there is page for WiFi credentials when PIN 9 on GND for 3 sec.
#define WIFI_PASS_DEFAULT   "DEIN_PASS"

// ========= MQTT Default =========
#define MQTT_HOST_DEFAULT   "192.168.1.61" //choose your IP where the MQTT service is running. I am using Docker on Raspi, all at the same IP. 
#define MQTT_PORT           1883

// ========= MQTT Topics =========
#define MQTT_BASE_TOPIC "garden/valve1"
#define TOPIC_STATE     MQTT_BASE_TOPIC "/state"
#define TOPIC_CMD       MQTT_BASE_TOPIC "/cmd"
#define TOPIC_CFG       MQTT_BASE_TOPIC "/cfg"
#define TOPIC_DIAG      MQTT_BASE_TOPIC "/diag"
#define TOPIC_PROG      MQTT_BASE_TOPIC "/prog"

// ========= OTA Web-Login =========
#define OTA_USER        "otauser"   //some improvements for next version in terms of data security
#define OTA_PASS        "superSecret123"

// ========= Zeit / NTP =========
#define NTP_SERVER_1    "pool.ntp.org"
#define NTP_TZ_OFFSET_S (7 * 3600)
#define NTP_RETRY_S     60
#define DAILY_RESET_H   4
#define DAILY_RESET_M   0

// ========= Default-Bewässerungsprogramm =========
#define IRR_DEFAULT_ENABLED     true
#define IRR_DEFAULT_START_H     6
#define IRR_DEFAULT_START_M     0
#define IRR_DEFAULT_DURATION_S  (10 * 60)
#define IRR_DEFAULT_MAX_RUN_S   (60 * 60)
