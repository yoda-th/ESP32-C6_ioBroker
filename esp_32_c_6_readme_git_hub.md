# ESP32-C6 Valve Controller – Bewässerungssteuerung für ioBroker & MQTT
*(Deutsch / English below)*

---

# 🇩🇪 Deutsch – Projektbeschreibung

Der **ESP32-C6 Valve Controller** steuert:
- ein motorisiertes 12-V-Ventil,
- einen 3/4" Flow-Sensor,
- optional eine Batterieüberwachung (ADC),
- automatische & manuelle Bewässerung,
- MQTT-Integration für ioBroker,
- OTA-Firmware-Updates per Browser.

Das System ist **fail-safe**: Das Ventil bleibt bei Boot, Watchdog-Reset, WiFi-Ausfall und OTA immer **geschlossen**.

---

## 🚀 Funktionen

### ✔ Motorventilsteuerung
- OPEN / CLOSE per Web, MQTT, ioBroker
- Sicheres Verhalten bei Fehlern

### ✔ Flow-Messung
- L/min
- Gesamtverbrauch in Liter
- Filter gegen Fehlpulse

### ✔ Batterieüberwachung (optional)
- ADC-Messung über Spannungsteiler (z. B. 51k/10k)

### ✔ MQTT-Integration
- State JSON → ioBroker
- Commands → OPEN / CLOSE / MODE_AUTO / MODE_MANUAL

### ✔ OTA („Over-The-Air“) Updates
- Firmware via Browser flashbar
- Kein USB-Kabel nötig

### ✔ Self-Healing & Watchdog
- WiFi-Reconnect
- Reset bei Ausfall > 5 Minuten
- Täglicher Reset um 04:00 (nur wenn nicht bewässert wird)

---

## 🔧 Hardware-Pins

| Funktion          | Pin | Beschreibung |
|------------------|-----|--------------|
| Ventil-Relais     | 4   | LOW=Auf, HIGH=Zu |
| Flow-Sensor       | 5   | Interrupt |
| Batterie-ADC      | 1   | 51k/10k Teiler |
| Status-LED        | 8   | Blinkindikator |
| Config-Button     | 2   | LOW beim Boot → Config-Portal |

---

## 📡 Erstkonfiguration (Config-Portal)

### **Config-Portal starten:**
1. Taste an GPIO2 gedrückt halten
2. Reset oder Einschalten
3. AP erscheint:
```
SSID: ValveConfig-XXXX
Passwort: config123
```
4. Verbinden
5. Browser öffnen → `http://192192.4.1`
6. WLAN + MQTT-IP eintragen
7. Speichern → Reboot

---

## 🌐 Weboberfläche

Aufruf:
```
http://<IP-des-ESP>/
```
Login (aus config.h):
```
OTA_USER
OTA_PASS
```
### Die Seite zeigt:
- Firmware-Version
- Ventilstatus
- Flow
- Batterie
- Irrigation / Auto-Mode
- WiFi-RSSI
- MQTT-Status
- Buttons OPEN / CLOSE
- OTA-Update-Link

---

## 🛠 OTA Update (Firmware per Browser)

### Schritte:
1. IP des ESP ermitteln
2. Browser öffnen → `http://<IP>/`
3. Login
4. Menü → Firmware Update (`/update`)
5. `.bin` Datei auswählen
6. Upload starten
7. ESP schließt Ventil → flasht → rebootet

### Nachprüfung:
- Ventil = **CLOSED**
- Neue Firmware-Version sichtbar
- MQTT-State aktualisiert

---

## 🧠 MQTT – Integration in ioBroker

### State JSON Beispiel:
```json
{
  "fw": "0.9.0",
  "device": "esp-valve-c6",
  "wifi_ip": "192.168.1.73",
  "wifi_rssi": -61,
  "valve": "CLOSED",
  "flow_lpm": 0.0,
  "flow_total_l": 123.4,
  "battery_v": 12.5,
  "irr_mode": "AUTO",
  "irr_running": false,
  "irr_remain_s": 0
}
```

### Commands:
| Befehl | Wirkung |
|--------|---------|
| `OPEN` | Ventil öffnen |
| `CLOSE` | Ventil schließen |
| `MODE_AUTO` | Automatik |
| `MODE_MANUAL` | Manuell |

---

## 🔒 Fail-Safe Verhalten

| Situation | Verhalten |
|-----------|-----------|
| Boot | Ventil ZU |
| OTA-Update | Ventil ZU |
| Watchdog Reset | Ventil ZU |
| WiFi Down > 5 min | Ventil ZU + Reboot |
| Daily Reset | Nur wenn nicht bewässert wird |

---

## 📦 Installation (PlatformIO / Arduino IDE)

Clone:
```
git clone https://github.com/yoda-th/ESP32-C6_ioBroker
```
Konfiguration in `config.h`:
- WLAN-Daten
- MQTT-Host
- Device-Name
- OTA-Zugangsdaten
- Pins

---

## 🧪 Testplan nach Installation
1. Weboberfläche erreichbar
2. Ventil OPEN/CLOSE
3. Flow prüfen
4. MQTT-State prüfen
5. OTA-Testupdate

---

# 🇬🇧 English – Project Description

The **ESP32-C6 Valve Controller** manages:
- 12-V motor valve
- Flow sensor
- Battery monitoring
- Automatic & manual irrigation
- Full MQTT integration (ioBroker)
- OTA firmware updating
- Safe-state architecture

The firmware is **fail-safe**: The valve is always **closed** on boot, OTA, watchdog resets or WiFi failures.

---

## 🚀 Features
- Valve control via Web/MQTT/ioBroker
- Flow monitoring
- Battery ADC monitoring
- Auto/manual irrigation
- OTA firmware updates
- Watchdog & WiFi self-healing

---

## 🔧 Hardware Pins

| Function | Pin | Description |
|----------|-----|-------------|
| Valve relay | 4 | LOW=Open, HIGH=Closed |
| Flow sensor | 5 | Interrupt |
| Battery ADC | 1 | 51k/10k divider |
| Status LED | 8 | Indicator |
| Config button | 2 | LOW on boot → config portal |

---

## 📡 Initial Setup

### Enter config mode:
1. Hold button on GPIO2
2. Reset device
3. ESP starts AP:
```
SSID: ValveConfig-XXXX
Password: config123
```
4. Connect
5. Open: `http://192.168.4.1`
6. Enter WiFi & MQTT host
7. Save & reboot

---

## 🌐 Web Interface

Open:
```
http://<IP>/
```
Login:
```
OTA_USER
OTA_PASS
```

---

## 🛠 OTA Update
1. Find ESP IP
2. Open Web UI
3. Navigate to `/update`
4. Select `.bin` file
5. Upload
6. ESP closes valve → flashes firmware → reboots

---

## 🧠 MQTT Integration

### State JSON
```json
{
  "fw": "0.9.0",
  "device": "esp-valve-c6",
  "wifi_ip": "192.168.1.73",
  "wifi_rssi": -61,
  "valve": "CLOSED",
  "flow_lpm": 0.0,
  "flow_total_l": 123.4,
  "battery_v": 12.5,
  "irr_mode": "AUTO",
  "irr_running": false,
  "irr_remain_s": 0
}
```

### Commands:
- `OPEN`
- `CLOSE`
- `MODE_AUTO`
- `MODE_MANUAL`

---

## 🔒 Fail-Safe Behavior
| Condition | Action |
|-----------|--------|
| Boot | Valve CLOSED |
| OTA update | Valve CLOSED |
| Watchdog reset | Valve CLOSED |
| WiFi offline > 5 min | Valve CLOSED + reboot |
| Daily reset | Only when no irrigation active |

---

## 📦 Installation
```
git clone https://github.com/yoda-th/ESP32-C6_ioBroker
```
Edit `config.h` before flashing.

---

## 🧪 Test Checklist
- Web UI reachable
- Valve control works
- Flow works
- MQTT works
- OTA update test

---

