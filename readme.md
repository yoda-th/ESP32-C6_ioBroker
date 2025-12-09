# 🌱 ESP32-C6 Smart Garden Valve Controller

Eine professionelle, firmware-basierte Bewässerungssteuerung für den **ESP32-C6 SuperMini**.
Das System bietet volle Integration in **ioBroker/Home Assistant** via MQTT, eine eigenständige Web-Oberfläche, OTA-Updates und umfassende Sicherheitsfunktionen.

## ✨ Features

### 💧 Steuerung & Automatik
* **Web-Interface:** Steuerung und Überwachung direkt im Browser (Responsive Design).
* **MQTT-Integration:** Vollständige Fernsteuerung und Statusmeldung (JSON).
* **Autarke Bewässerung:** Funktioniert auch ohne WiFi/ioBroker basierend auf internem Timer.
* **Durchflussmessung:** Echtzeit-Anzeige (L/min) und Gesamtzähler (Total Liter).

### 🛡️ Sicherheit & Alarm-Logik
* **Leckage-Erkennung:** Alarm, wenn das Ventil geschlossen ist (`CLOSED`), aber der Flow-Sensor Wasserfluss misst (> 0.5 L/min).
* **Stagnations-Warnung:** Alarm, wenn das Ventil länger als **30 Stunden** nicht geöffnet wurde (Schutz vor Festsetzen).
* **Watchdog-Schutz:** Hardware-Watchdog überwacht das System; wird für OTA-Updates dynamisch deaktiviert, um Abstürze zu verhindern.
* **Last Will & Testament (LWT):** Zuverlässige Online/Offline-Erkennung im MQTT-Broker.

### ⚙️ Hardware-Support
* **Chip:** ESP32-C6 SuperMini (RISC-V).
* **Ventil:** Unterstützung für 5V-Relais (Low-Trigger / Open-Drain Logik für 3.3V ESPs).
* **Power:** Batterieüberwachung (ADC) mit Spannungsumrechnung.

---

## 🔌 Pinbelegung (Pinout)

Basierend auf der `config.h`:

| Komponente | GPIO Pin | Beschreibung |
| :--- | :--- | :--- |
| **Ventil Relais** | `GPIO 4` | Low-Active (geschaltet gegen GND) |
| **Flow Sensor** | `GPIO 5` | Pulse-Input (Interrupt) |
| **Batterie ADC** | `GPIO 1` | Spannungsmessung (Analog) |
| **Status LED** | `GPIO 8` | Heartbeat / Status |
| **Config Button**| `GPIO 2` | (Optional) Input |

---

## 📡 MQTT Schnittstelle

Die Firmware nutzt ein **X-Macro System**, um Topics dynamisch zu generieren. Die Struktur wird zentral in `config.h` definiert und automatisch auf die Web-Oberfläche und API übertragen.

**Basis-Topic (Default):** `garden/valve1`

| Funktion | Topic Endung | Richtung | Beschreibung |
| :--- | :--- | :--- | :--- |
| **Status** | `/stat` | `ESP -> Broker` | JSON mit Ventil, Flow, Batterie, WLAN-Signal, Fehlerstatus. |
| **Kommando** | `/cmnd` | `Broker -> ESP` | Befehle als Text: `OPEN`, `CLOSE`, `MODE_AUTO`, `MODE_MANUAL`. |
| **LWT** | `/lwt` | `ESP -> Broker` | Verbindungsstatus: `Online` oder `Offline` (Retained). |
| **Diagnose** | `/diag` | `ESP -> Broker` | Klartext-Fehlermeldungen (z.B. "ALARM: LEAK DETECTED!"). |
| **Config** | `/cfg` | `ESP <-> Broker` | Abrufen/Setzen der Konfiguration. |
| **Programm** | `/prog` | `ESP <-> Broker` | Setzen der Bewässerungszeiten. |

### Beispiel Status JSON (`/stat`)
```json
{
  "fw": "0.9.8-C6",
  "valve": "CLOSED",
  "flow_lpm": 0.00,
  "battery_v": 12.4,
  "last_diag": "OK",
  "mqtt_lwt": "garden/valve1/lwt"
}
🖥️ Web Interface
Das Web-Interface ist unter der IP-Adresse des ESP erreichbar (z.B. http://192.168.x.xx).

Funktionen:

Live Status: Anzeige von Ventil, Durchfluss, Batterie und RSSI.

Manuelle Kontrolle: Buttons für OPEN / CLOSE.

Diagnose: Anzeige aktueller Alarme (Rot/Fett) mit Quittierungs-Button (Alarm bestätigen).

MQTT Info: Dynamische Liste aller aktiven MQTT-Topics (ausgelesen aus config.h).

OTA Update: Hochladen neuer Firmware (firmware.bin) direkt über den Browser.

🛠️ Installation & Kompilieren
Das Projekt basiert auf PlatformIO (VS Code).

Repository klonen.

src/config.h anpassen:

WLAN-Zugangsdaten (WIFI_SSID_DEFAULT, WIFI_PASS_DEFAULT).

MQTT-Broker IP (MQTT_HOST_DEFAULT).

Projekt bauen (Build Button ✓).

Erstmalig per USB flashen (Upload Pfeil →).

Zukünftige Updates bequem über das Web-Interface (/update) einspielen.

🐛 Debugging & Logs
Serial Monitor: Baudrate 115200.

Web: Alarme werden auf der Startseite rot angezeigt und im internen Speicher gehalten.

MQTT: Abonniere garden/valve1/diag für Echtzeit-Fehlermeldungen.

Version: 0.9.8-C6