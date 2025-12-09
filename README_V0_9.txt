ESP32-C6 Valve Firmware V0.9.0

Diese Version basiert auf V0.8.0, mit folgenden Schwerpunkten:
- Verbesserte Flow-Auswertung (Filter, Entprellung, Plausibilitäts-Checks)
- Robusteres Bewässerungsverhalten (Irrigation-Modul, Anomaliechecks)
- Verfeinertes WiFi-Reconnect-Verhalten mit Zähler
- Täglicher Reset wird nicht während laufender Bewässerung ausgeführt
- Web-API erweitert um Irrigation-Informationen

Alle geänderten Dateien sind im Kopf mit
'// Firmware V0.9.0 – based on V0.8.0, modified in this version.'
gekennzeichnet und enthalten zusätzliche Kommentare mit 'V0.9 CHANGE:'.
