ESP32-C6 Valve Firmware V0.8.0

Struktur:
- main.cpp
- config.h
- logger.*, wifi_module.*, wifi_config_module.*, mqtt_module.*
- valve_module.*, flow_module.*, battery_module.*
- time_module.*, watchdog_module.*, irrigation_module.*
- web_module.*

Abhängigkeiten (Arduino IDE):
- Board: ESP32 (inkl. C6)
- Libraries:
  - WiFi (Core)
  - WebServer (Core)
  - Update (Core)
  - Preferences (Core)
  - time.h / configTime (Core)
  - esp_task_wdt (Core)
  - PubSubClient (über Bibliotheksverwalter)

Hinweis:
- Partition Scheme mit OTA-Unterstützung wählen.
- WIFI_SSID_DEFAULT, WIFI_PASS_DEFAULT, MQTT_HOST_DEFAULT in config.h anpassen.
