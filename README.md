# CCARemote – Arduino Bibliothek

Flexible Steuerung von Mikrocontrollern über Bluetooth Low Energy (BLE) oder WLAN (WiFi). Die erforderliche App **CCA Remote** ist für Android und iOS kostenlos verfügbar.
Dieses Projekt wurde von der HTL Anichstraße (Abteilung Wirtschaftsingenieure – Betriebsinformatik) entwickelt.

Unterstützte Protokolle:
- **Bluetooth Low Energy (BLE)**
- **WiFi (WLAN-Hotspot + HTTP)**
- **MQTT** (in Arbeit)

---

## Installation

1. Den Ordner `CCARemote-Arduino` als `CCARemote` in den Arduino-Bibliotheksordner kopieren  
   (`Dokumente/Arduino/libraries/CCARemote`)

---

## Klassen im Überblick

| Klasse | Protokoll | `#include` |
|---|---|---|
| `CCARemoteBLE` | Bluetooth Low Energy | `#include <CCARemoteBLE.h>` |
| `CCARemoteWiFi` | WiFi Hotspot (HTTP) | `#include <CCARemoteWiFi.h>` |

> Beide Klassen haben dieselbe API – wer BLE kennt, kann sofort auf WiFi wechseln.

---

## CCARemoteBLE – Bluetooth

```cpp
#include <CCARemoteBLE.h>

CCARemoteBLE remote("MeinName");  // Gerätename: "remote-MeinName"

// --- Variablen oben deklarieren ---
bool ledAn     = false;  // Button / Switch
int  helligkeit = 0;     // Slider (0–255)

void setup() {
  remote.begin("12345678");  // BLE AUTH Passwort (leer lassen = ohne AUTH Passwort)

  // Variable mit App-Befehl verknüpfen (empfohlen)
  remote.receive("ledAn",     ledAn);
  remote.receive("helligkeit", helligkeit);
}

void loop() {
  remote.handle();  // Immer in loop() aufrufen – aktualisiert Variablen!

  // Variablen direkt verwenden
  digitalWrite(LED_PIN, ledAn);
  analogWrite(PWM_PIN, helligkeit);
}
```

**`begin(BLEPassword)`** – BLE AUTH-Passwort kann in der App unter Einstellungen gesetzt werden.

---

## CCARemoteWiFi – WiFi Hotspot

```cpp
#include <CCARemoteWiFi.h>

CCARemoteWiFi remote("MeinName");

// --- Variablen oben deklarieren ---
bool ledChange = false;

void setup() {
  remote.begin("12345678");  // WLAN-Passwort (leer lassen = offenes Netz)

  remote.receive("ledChange", ledChange);
}

void loop() {
  remote.handle();

  digitalWrite(LED_PIN, ledChange);
}
```

Der ESP32 erstellt einen WLAN-Hotspot mit dem Namen `remote-MeinName`.  
Die App verbindet sich damit und sendet Befehle über HTTP.

**`begin(wifiPassword)`** – WiFi-Hotspot starten. Passwort weglassen oder leer lassen für ein offenes Netzwerk.

---

## API-Referenz

### `begin()` – Verbindung starten

| Klasse | Aufruf |
|---|---|
| `CCARemoteBLE` | `remote.begin()` oder `remote.begin("passwort")` |
| `CCARemoteWiFi` | `remote.begin()` oder `remote.begin("passwort")` |

---

### `receive()` – Variable mit App verknüpfen *(empfohlen)*

Einfachste Methode: Variable oben deklarieren, einmal binden – `remote.handle()` aktualisiert sie automatisch.

```cpp
// Variablen deklarieren
bool  ledAn      = false;
int   helligkeit = 0;
float temperatur = 0.0;

void setup() {
  remote.begin();

  // Typen werden automatisch erkannt
  remote.receive("ledAn",      ledAn);       // bool  – für Button, Switch
  remote.receive("helligkeit", helligkeit);  // int   – für Slider, Zahlenwerte
  remote.receive("sollTemp",   temperatur);  // float – für Dezimalwerte
}

void loop() {
  remote.handle();  // aktualisiert ledAn, helligkeit, temperatur

  // Variablen ganz normal verwenden
  digitalWrite(LED_PIN, ledAn);
  analogWrite(PWM_PIN, helligkeit);
}
```

| Typ | Verwendung |
|---|---|
| `bool` | Button (gedrückt = true), Switch (an = true) |
| `int` | Slider (0–255), ganze Zahlen |
| `float` | Schieberegler mit Dezimalwerten |

---

### `onCommand()` – Callback bei Empfang *(für komplexe Logik)*

Wenn beim Empfang eines Wertes sofort etwas ausgeführt werden soll:

```cpp
// Befehl ohne Wert (z. B. Button-Druck)
remote.onCommand("ledToggle", []() {
  digitalWrite(LED_PIN, !digitalRead(LED_PIN));
});

// Befehl mit Wert (z. B. Slider, Texteingabe)
remote.onCommand("helligkeit", [](String wert) {
  analogWrite(PWM_PIN, wert.toInt());
});
```

---

### `send()` – Werte in der App anzeigen

```cpp
// Schlüssel + Wert (empfohlen)
remote.send("temperatur", 23.5);
remote.send("status", "aktiv");
remote.send("zaehler", 42);

// Float mit bestimmten Nachkommastellen
remote.send("spannung", 3.7, 2);  // "3.70"
```

| Überladung | Beschreibung |
|---|---|
| `send(key, String)` | Text-Wert |
| `send(key, int)` | Ganzzahl |
| `send(key, float)` | Dezimalzahl (1 Nachkommastelle) |
| `send(key, float, int)` | Dezimalzahl mit gewünschten Nachkommastellen |

---

### `debug()` – Seriellen Monitor aktivieren

Aktiviert die Ausgabe empfangener und/oder gesendeter Werte im Seriellen Monitor. Ruft automatisch `Serial.begin()` mit der angegebenen Baudrate auf.

```cpp
remote.debug();                        // IN + OUT, 9600 Baud (Standard)
remote.debug(CCA_DEBUG_ALL, 115200);   // IN + OUT, 115200 Baud
remote.debug(CCA_DEBUG_IN);           // nur empfangene Werte (IN)
remote.debug(CCA_DEBUG_OUT);          // nur gesendete Werte (OUT)
remote.debug(CCA_DEBUG_OFF);          // Debug-Modus deaktivieren
```

| Modus | Wert | Beschreibung |
|---|---|---|
| `CCA_DEBUG_OFF` | `0` | Kein Debug-Output |
| `CCA_DEBUG_IN`  | `1` | Empfangene Werte ausgeben (`[CCA] IN  key = wert`) |
| `CCA_DEBUG_OUT` | `2` | Gesendete Werte ausgeben (`[CCA] OUT key = wert`) |
| `CCA_DEBUG_ALL` | `3` | Empfangene und gesendete Werte ausgeben |

| Parameter | Typ | Standard | Beschreibung |
|---|---|---|---|
| `mode` | `CCADebugMode` | `CCA_DEBUG_ALL` | Welche Richtung(en) ausgegeben werden |
| `baudRate` | `unsigned long` | `9600` | Baudrate für `Serial.begin()` |

> **Hinweis:** `debug()` muss **vor** `remote.begin()` aufgerufen werden, damit die Baudrate korrekt gesetzt wird. `Serial.begin()` muss dann **nicht** zusätzlich in `setup()` aufgerufen werden.

---

### Gerätename und Prefix anpassen

Standardmäßig wird dem Gerätenamen automatisch der Prefix `CCA-` vorangestellt. Der Prefix kann frei angepasst oder ganz weggelassen werden:

```cpp
CCARemoteWiFi remote("Roboter");             // → "CCA-Roboter"  (Standard)
CCARemoteWiFi remote("Roboter", "HTL-");     // → "HTL-Roboter"
CCARemoteWiFi remote("Roboter", "");         // → "Roboter"  (kein Prefix)
```

Der zweite Parameter gilt für alle Klassen (`CCARemoteBLE`, `CCARemoteWiFi`).

---

## `handle()` – Verarbeitung (zwingend in `loop()`)

```cpp
void loop() {
  remote.handle();  // Empfangene Befehle verarbeiten, Verbindung pflegen
}
```

---

### `isConnected()` – Verbindungsstatus prüfen

```cpp
if (remote.isConnected()) {
  // App ist verbunden
}
```

---

## Vollständiges Beispiel (BLE)

```cpp
#include <CCARemoteBLE.h>

CCARemoteBLE remote("MeinName");  // Namen hier anpassen!

const int LED_BUTTON = 18;
const int LED_SLIDER = 19;
const int LED_SWITCH = 5;

// --- Variablen (werden von remote.handle() automatisch aktualisiert) ---
// Die Namen müssen mit den Variablennamen in der App übereinstimmen!
bool changeLed  = false;  // Button
int  brightness = 0;      // Slider (0–255)
bool ledSwitch  = false;  // Switch

void setup() {
  pinMode(LED_BUTTON, OUTPUT);
  pinMode(LED_SWITCH, OUTPUT);
  remote.begin();

  remote.receive("changeLed",  changeLed);
  remote.receive("brightness", brightness);
  remote.receive("ledSwitch",  ledSwitch);
}

void loop() {
  remote.handle();  // Variablen aktualisieren

  // Variablen direkt verwenden
  digitalWrite(LED_BUTTON, changeLed);
  analogWrite(LED_SLIDER,  brightness);
  digitalWrite(LED_SWITCH, ledSwitch ? HIGH : LOW);

  // Alle 2 Sekunden Status senden
  static unsigned long letzterSend = 0;
  if (millis() - letzterSend >= 2000) {
    letzterSend = millis();
    remote.send("uptime", (int)(millis() / 1000));
  }
}
```

---

## Voraussetzungen

- **Board:** ESP32 (beliebiges Modell)
- **Arduino IDE:** 2.x empfohlen
- **ESP32-Paket:** Boardverwalter → `esp32` von Espressif

---

## Hinweis
Diese Bibliothek basiert auf der Diplomarbeit von Lucas E. und Enes D. an der HTL Anichstraße. Die Nutzung erfolgt auf eigene Verantwortung – es wird keine Gewährleistung für Richtigkeit, Vollständigkeit oder Eignung für einen bestimmten Zweck übernommen.
