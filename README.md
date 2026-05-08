# CCARemote – Arduino Bibliothek

Flexible Steuerung von Mikrocontrollern über Bluetooth Low Energy (BLE) oder WLAN (WiFi). Die erforderliche App **CCA Remote** ist für Android und iOS kostenlos verfügbar.
Dieses Projekt wurde von der HTL Anichstraße (Abteilung Wirtschaftsingenieure – Betriebsinformatik) entwickelt.

Unterstützte Protokolle:
- **Bluetooth Low Energy (BLE)**
- **WiFi (WLAN-Hotspot + HTTP)**
- **MQTT** (in Arbeit)

Unterstützte Hardware:
- **ESP32** – natives BLE, WiFi, MQTT
- **ESP8266** - WiFi, MQTT
- **Arduino Uno / Nano** – BLE über HM-10-Modul (SoftwareSerial)
- **Raspberry Pi Pico 2W** – natives BLE, WiFi, MQTT (erfordert die [CCARemote-MicroPython](https://github.com/ccaprojects/CCARemote-MicroPython) Bibliothek)

---

## Installation

1. Den Ordner `CCARemote-Arduino` als `CCARemote` in den Arduino-Bibliotheksordner kopieren  
   (`Dokumente/Arduino/libraries/CCARemote`)

---

## Klassen im Überblick

| Klasse | Protokoll | `#include` | Hardware |
|---|---|---|---|
| `CCARemoteBLE` | Bluetooth Low Energy | `#include <CCARemoteBLE.h>` | ESP32 oder Arduino + HM-10 |
| `CCARemoteWiFi` | WiFi Hotspot (HTTP) | `#include <CCARemoteWiFi.h>` | ESP32, ESP8266 |

> `CCARemoteBLE` erkennt die Zielplattform **automatisch beim Kompilieren** und wählt die passende Implementierung – der Sketch-Code bleibt auf beiden Plattformen identisch.

---

## CCARemoteBLE – Bluetooth

### ESP32 (natives BLE)

```cpp
#include <CCARemoteBLE.h>

CCARemoteBLE remote("MeinName");

bool ledAn      = false;
int  helligkeit = 0;

void setup() {
  remote.begin("12345678");  // BLE AUTH-Passwort (leer = ohne Passwort)

  remote.receive("ledAn",      ledAn);
  remote.receive("helligkeit", helligkeit);
}

void loop() {
  remote.handle();

  digitalWrite(LED_PIN, ledAn);
  analogWrite(PWM_PIN,  helligkeit);
}
```

### Arduino Uno / Nano mit HM-10-Modul

```cpp
#include <CCARemoteBLE.h>

// Standard: RX=10, TX=11, 9600 Baud
CCARemoteBLE remote("MeinName");

// Eigene Pins / Baudrate:
// CCARemoteBLE remote("MeinName", "CCA-", 8, 9, 9600);

bool ledAn      = false;
int  helligkeit = 0;

void setup() {
  remote.begin("12345678");  // BLE AUTH-Passwort (leer = ohne Passwort)

  remote.receive("ledAn",      ledAn);
  remote.receive("helligkeit", helligkeit);
}

void loop() {
  remote.handle();

  digitalWrite(LED_PIN, ledAn);
  analogWrite(PWM_PIN,  helligkeit);
}
```

#### Konstruktor-Parameter (nur Arduino / HM-10)

```cpp
CCARemoteBLE remote(name, prefix, rxPin, txPin, baudRate);
```

| Parameter | Typ | Standard | Beschreibung |
|---|---|---|---|
| `name` | `String` | – | Gerätename (wird mit Prefix kombiniert) |
| `prefix` | `String` | `"CCA-"` | Prefix für den Gerätenamen |
| `rxPin` | `uint8_t` | `10` | Arduino-Pin → HM-10 TX |
| `txPin` | `uint8_t` | `11` | Arduino-Pin → HM-10 RX |
| `baudRate` | `uint32_t` | `9600` | Baudrate des HM-10-Moduls |

#### HM-10 Verdrahtung

| HM-10 | Arduino Uno / Nano |
|---|---|
| VCC | 3,3 V oder 5 V (je nach Modul) |
| GND | GND |
| TXD | Pin 10 (RX) |
| RXD | Pin 11 (TX) – bei 5-V-Arduinos Spannungsteiler empfohlen¹ |

> ¹ Der HM-10 arbeitet mit 3,3-V-Logik. Ein Spannungsteiler (z. B. 10 kΩ / 20 kΩ) schützt den Eingang des Moduls vor den 5-V-Pegeln des Arduino.

#### Hinweise zum HM-10

- Die Bibliothek konfiguriert das Modul beim Start automatisch (Name, Slave-Modus, Reset).
- Bei Auth-Fehler kann das HM-10 die Verbindung nicht aktiv trennen; die App übernimmt das und zeigt die Fehlermeldung an.
- Gerätenamen werden auf 12 Zeichen begrenzt (HM-10-Firmware-Limit).
- Getestete Firmware: v5xx. Bei abweichenden Verbindungs-Events (z. B. `AT+CONNECTED` statt `OK+CONN`) ggf. Firmware updaten.

---

## CCARemoteWiFi – WiFi Hotspot

```cpp
#include <CCARemoteWiFi.h>

CCARemoteWiFi remote("MeinName");

bool ledChange = false;

void setup() {
  remote.begin("12345678");  // WLAN-Passwort (leer = offenes Netz)

  remote.receive("ledChange", ledChange);
}

void loop() {
  remote.handle();

  digitalWrite(LED_PIN, ledChange);
}
```

ESP32 und ESP8266 erstellen einen WLAN-Hotspot mit dem Namen `CCA-MeinName`.  
Die App verbindet sich damit und sendet Befehle über HTTP.

**`begin(wifiPassword)`** – Passwort weglassen oder leer lassen für ein offenes Netzwerk.

> **Hinweis Passwortlänge:** WPA2 erfordert mindestens **8 Zeichen**. Ein kürzeres Passwort führt dazu, dass der Hotspot nicht gestartet wird. Die Library gibt in diesem Fall eine Fehlermeldung im Seriellen Monitor aus.

---

## Elemente und Typen

| Element | Typ | Richtung | Hinweis |
|---|---|---|---|
| Button | `bool` | App → Arduino | `true` = gedrückt |
| Switch | `bool` | App → Arduino | `true` = ein |
| Slider | `int` | App → Arduino | Bereich in der App einstellbar (Standard 0–255) |
| Joystick | `int` | App → Arduino | X und Y als separate Element-IDs |
| Input | `String` | App → Arduino | Freier Text |
| Display | `send()` | Arduino → App | Messwert anzeigen |
| Gauge / Bar | `send()` | Arduino → App | Balken / Kreisbogen |
| Chart | `send()` | Arduino → App | Liniendiagramm |
| Status-LED | `send()` | Arduino → App | Ganzzahl 0–3 |
| Label | `send()` | Arduino → App | Text (optional, nur wenn Element-ID gesetzt) |

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
bool  ledAn      = false;
int   helligkeit = 0;
float temperatur = 0.0;

void setup() {
  remote.begin();

  remote.receive("ledAn",      ledAn);       // bool  – für Button, Switch
  remote.receive("helligkeit", helligkeit);  // int   – für Slider, Zahlenwerte
  remote.receive("sollTemp",   temperatur);  // float – für Dezimalwerte
}

void loop() {
  remote.handle();

  digitalWrite(LED_PIN, ledAn);
  analogWrite(PWM_PIN, helligkeit);
}
```

| Typ | Verwendung |
|---|---|
| `bool` | Button (gedrückt = true), Switch (an = true) |
| `int` | Slider (0–255), Joystick-Achse (−255 – +255) |
| `float` | Schieberegler mit Dezimalwerten |
| `String` | Texteingabe (Input) |

> **Joystick:** Jede Achse hat eine eigene Element-ID. X- und Y-Variable werden separat mit `remote.receive()` verknüpft.
>
> ```cpp
> int axisX = 0, axisY = 0;
> remote.receive("axisX", axisX);  // Joystick X (−255 – +255)
> remote.receive("axisY", axisY);  // Joystick Y (−255 – +255)
> ```

---

### `onCommand()` – Callback bei Empfang *(für komplexe Logik)*

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

> **Hinweis – Label-Element:** Neben Display-, Gauge-, Chart- und LED-Elementen kann auch das **Label**-Element Werte empfangen. `remote.send("label1", "Text")` aktualisiert den angezeigten Text des Labels dynamisch. Die Element-ID muss dazu im Label-Editor der App eingetragen sein.

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

> **Hinweis:** `debug()` muss **vor** `remote.begin()` aufgerufen werden.

---

### Gerätename und Prefix anpassen

```cpp
CCARemoteBLE remote("Roboter");           // → "CCA-Roboter"  (Standard)
CCARemoteBLE remote("Roboter", "HTL-");   // → "HTL-Roboter"
CCARemoteBLE remote("Roboter", "");       // → "Roboter"  (kein Prefix)
```

---

### `handle()` – Verarbeitung (zwingend in `loop()`)

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
// Arduino + HM-10 mit eigenen Pins: CCARemoteBLE remote("MeinName", "CCA-", 8, 9);

const int LED_BUTTON = 18;
const int LED_SLIDER = 19;
const int LED_SWITCH = 5;

bool changeLed  = false;
int  brightness = 0;
bool ledSwitch  = false;

void setup() {
  pinMode(LED_BUTTON, OUTPUT);
  pinMode(LED_SWITCH, OUTPUT);
  remote.begin();

  remote.receive("changeLed",  changeLed);
  remote.receive("brightness", brightness);
  remote.receive("ledSwitch",  ledSwitch);
}

void loop() {
  remote.handle();

  digitalWrite(LED_BUTTON, changeLed);
  analogWrite(LED_SLIDER,  brightness);
  digitalWrite(LED_SWITCH, ledSwitch ? HIGH : LOW);

  static unsigned long letzterSend = 0;
  if (millis() - letzterSend >= 2000) {
    letzterSend = millis();
    remote.send("uptime", (int)(millis() / 1000));
  }
}
```

---

## Voraussetzungen

### ESP32

- **Board:** ESP32 (beliebiges Modell)
- **Arduino IDE:** 2.x empfohlen
- **ESP32-Paket:** Boardverwalter → `esp32` von Espressif

### Arduino Uno / Nano + HM-10

- **Board:** Arduino Uno oder Nano (ATmega328P)
- **Arduino IDE:** 2.x empfohlen
- **Bibliothek:** `SoftwareSerial` (im Arduino IDE vorinstalliert)
- **Modul:** HM-10 BLE-Modul (CC2540 / CC2541 Chip, Firmware v5xx)

---

## Hinweis
Diese Bibliothek basiert auf der Diplomarbeit von Lucas E. und Enes D. an der HTL Anichstraße. Die Nutzung erfolgt auf eigene Verantwortung – es wird keine Gewährleistung für Richtigkeit, Vollständigkeit oder Eignung für einen bestimmten Zweck übernommen.
