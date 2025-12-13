💾 Implementierungs-Guide: Das 2-Ebenen-Zähler-Modell
🎯 Das Ziel
Wir wollen den gesamten Wasserverbrauch dauerhaft speichern ("Total Counter"), ohne den Flash-Speicher (NVS) des ESP32 zu zerstören. Flash-Speicher hat eine begrenzte Lebensdauer (ca. 100.000 Schreibzyklen). Würden wir jeden Liter sofort speichern, wäre der Chip nach wenigen Monaten defekt.

💡 Das Konzept
Wir teilen den Zähler in zwei Variablen auf:

Zähler A (RAM / Flüchtiger Speicher): Zählt kleine Mengen (0 bis 100 Liter). Wird nicht permanent gespeichert.

Zähler B (Flash / Permanenter Speicher): Zählt nur volle 100-Liter-Blöcke. Wird nur geschrieben, wenn A überläuft.

Ergebnis: Statt 100 Schreibvorgängen (für 100 Liter) haben wir nur noch einen einzigen. Die Lebensdauer erhöht sich um den Faktor 100.

🛠️ Schritt 1: src/settings_module erweitern
Wir benötigen zwei Speicherplätze im NVS (total_liters_A und total_liters_B) und spezielle Funktionen.

1.1 In src/settings_module.h hinzufügen:

C++

// --- Gesamtzähler Logik (Flash-Schonend) ---

// Gibt den echten Gesamtwert zurück (A + B) für Anzeige/MQTT
float settingsGetTotalLiters();

// Setzt alles auf 0 (für Reset-Button)
void settingsSetTotalLiters(float l);

// Interne Getter/Setter für die 2-Ebenen-Logik
float settingsGetTotalLitersA();
void settingsSetTotalLitersA(float l);
float settingsGetTotalLitersB();
void settingsSetTotalLitersB(float l);

// Speichert explizit den Zählerstand (wird vom Flow-Modul gerufen)
void settingsSaveTotalCounter();
1.2 In src/settings_module.cpp implementieren:

C++

// Globale Variablen oben definieren
static float totalLitersA = 0.0; // Der kleine Zähler
static float totalLitersB = 0.0; // Der große Block-Zähler

// In settingsLoad() ergänzen:
void settingsLoad() {
    prefs.begin("valve-cfg", true);
    // ... andere Werte ...
    totalLitersA = prefs.getFloat("total_liters_A", 0.0);
    totalLitersB = prefs.getFloat("total_liters_B", 0.0);
    prefs.end();
}

// Neue Speicher-Funktion (schreibt NUR die Zähler)
void settingsSaveTotalCounter() {
    prefs.begin("valve-cfg", false);
    prefs.putFloat("total_liters_A", totalLitersA);
    prefs.putFloat("total_liters_B", totalLitersB);
    prefs.end();
}

// === Implementierung der Getter/Setter ===

float settingsGetTotalLitersA() { return totalLitersA; }
void settingsSetTotalLitersA(float l) { totalLitersA = l; }

float settingsGetTotalLitersB() { return totalLitersB; }
void settingsSetTotalLitersB(float l) { totalLitersB = l; }

// Öffentlicher Getter (Summe)
float settingsGetTotalLiters() { 
    return totalLitersB + totalLitersA; 
}

// Öffentlicher Reset
void settingsSetTotalLiters(float l) { 
    // Wir nutzen das Argument 'l' meist nur für 0.0 Reset
    totalLitersA = l;
    totalLitersB = 0.0;
    settingsSaveTotalCounter(); // Sofort speichern
}
🛠️ Schritt 2: src/flow_module.cpp anpassen
Hier passiert die eigentliche "Magie". Wir zählen im Loop hoch und prüfen auf Überlauf.

In flowLoop():

C++

#include "settings_module.h" // Nicht vergessen!

// Konstante definieren (z.B. oben in der Datei)
const float OVERFLOW_THRESHOLD = 100.0; // Speichern alle 100 Liter

void flowLoop() {
    unsigned long now = millis();
    if (now - lastCalc > 1000) { 
        // ... (Berechnung von flowLpm wie bisher) ...
        
        // Liter in dieser Sekunde berechnen
        float litersThisSec = (flowLpm / 60.0);
        
        // 1. Session Counter (RAM) erhöhen (für Web-Anzeige "Aktueller Lauf")
        sessionLiters += litersThisSec;
        
        // 2. 2-Ebenen-Logik für Total Counter:
        
        // A) Kleinen Zähler (A) im RAM erhöhen
        float currentA = settingsGetTotalLitersA();
        currentA += litersThisSec;
        settingsSetTotalLitersA(currentA);

        // B) Prüfen ob A voll ist (Überlauf)
        if (settingsGetTotalLitersA() >= OVERFLOW_THRESHOLD) {
            
            // Großen Zähler (B) um 100 erhöhen
            float currentB = settingsGetTotalLitersB();
            currentB += OVERFLOW_THRESHOLD; 
            settingsSetTotalLitersB(currentB);
            
            // Kleinen Zähler (A) reduzieren (Rest behalten für Präzision)
            settingsSetTotalLitersA(settingsGetTotalLitersA() - OVERFLOW_THRESHOLD); 
            
            // JETZT SPEICHERN (Nur 1x alle 100 Liter!)
            settingsSaveTotalCounter();
            
            logInfo("Flash Save: Total B updated to " + String(currentB, 0));
        }
        
        lastCalc = now;
    }
}
🛠️ Schritt 3: Nutzung (Web & MQTT)
Da wir die Funktion settingsGetTotalLiters() (die Summe aus A+B) gebaut haben, muss am restlichen Code kaum etwas geändert werden.

MQTT / JSON: Wenn Sie flowGetTotalLiters() aufrufen (was wiederum settingsGetTotalLiters() aufruft), erhalten Sie immer den korrekten Gesamtwert (z.B. 12345.6 Liter).

Webseite: Zeigt ebenfalls die Summe an.

Reset: Der Reset-Button auf der Webseite ruft settingsSetTotalLiters(0.0) auf, was A und B nullt und einmalig speichert.

✅ Checkliste für später
[ ] src/settings_module.h erweitern (Definitionen).

[ ] src/settings_module.cpp erweitern (Logik für A/B Variablen).

[ ] src/flow_module.cpp anpassen (Überlauf-Logik und OVERFLOW_THRESHOLD einbauen).

[ ] Testen: Setzen Sie OVERFLOW_THRESHOLD kurzzeitig auf 1.0 (1 Liter), um zu sehen, ob das Speichern funktioniert (Log-Ausgabe prüfen). Danach auf 100.0 oder 1000.0 setzen.