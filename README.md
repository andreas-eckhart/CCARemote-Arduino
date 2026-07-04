# CCARemote – Arduino Bibliothek

Flexible Steuerung von Mikrocontrollern über Bluetooth Low Energy (BLE) oder WLAN (WiFi). Die erforderliche App **CCA Remote** ist für Android und iOS kostenlos verfügbar.
Dieses Projekt wurde von der HTL Anichstraße entwickelt.

Unterstützte Protokolle:
- **Bluetooth Low Energy (BLE)**
- **WiFi (WLAN-Hotspot + TCP)**

Unterstützte Hardware:

| MCU | Bluetooth | WiFi |
|---|:---:|:---:|
| ESP32 | 🟢 | 🟢 |
| ESP8266 | 🟢 ¹ | 🟢 |
| Arduino Uno / Nano | 🟢 ¹ | ❌ |
| Raspberry Pi Pico 2W | 🟢 ² | 🟢 ² |

¹ Externes HM-10 BLE-Modul erforderlich.  
² Erfordert die [CCARemote-MicroPython](https://github.com/ccaprojects/CCARemote-MicroPython) Bibliothek.

---

## Installation

Die Bibliothek ist im Arduino IDE und PlatformIO Bibliothek-Manager unter dem Namen "CCARemote" verfügbar. Die Bibliothek beinhaltet zahlreiche Beispiele.

---

## Schnellstart

4 Zeilen konfigurieren, `#include <CCARemote.h>` – fertig. Das `remote`-Objekt wird automatisch angelegt und `begin()` hat keine Parameter:

```cpp
// ---- Konfiguration – hier anpassen! -----------------------
#define DEVICE_NAME  "MeinName"    // Gerätename (wird als "CCA-MeinName" angezeigt)
#define CONNECTION   CCA_BLE       // CCA_BLE  oder  CCA_WIFI
#define PASSWORD     ""            // Passwort (WiFi: min. 8 Zeichen / leer = ohne)
#define DEBUG_LEVEL  CCA_DEBUG_ALL // CCA_DEBUG_OFF / _IN / _OUT / _ALL

// Optional – nur setzen wenn Standardwert nicht passt:
// #define DEVICE_PREFIX "XYZ-"   // Standard: "CCA-"
// #define TCP_PORT      4211     // Standard: 4210  (nur WiFi)
// #define BAUD_RATE     9600     // Standard: 115200
// -----------------------------------------------------------

#include <CCARemote.h>

const int LED_PIN = 2;  // GPIO-Pin anpassen (ESP32 DevKit: 2, Uno/Nano: 13)

bool switch_led = false;

void setup() {
  remote.begin();

  remote.receive("switch1", switch_led);
  pinMode(LED_PIN, OUTPUT);
}

void loop() {
  remote.handle();

  if (remote.isConnected()) {
    digitalWrite(LED_PIN, switch_led ? HIGH : LOW);
  } else {
    digitalWrite(LED_PIN, LOW);
  }
}
```

Um zwischen BLE und WiFi zu wechseln, nur `CONNECTION` ändern – der restliche Sketch-Code bleibt identisch.

---

## Konfiguration (#defines)

| Define | Standard | Beschreibung |
|---|---|---|
| `DEVICE_NAME` | – | **Pflicht.** Gerätename, wird als `CCA-<name>` angezeigt |
| `CONNECTION` | – | **Pflicht.** `CCA_BLE` oder `CCA_WIFI` |
| `PASSWORD` | `""` | Passwort (BLE: AUTH-Passwort, WiFi: WPA2-Passwort ≥ 8 Zeichen) |
| `DEBUG_LEVEL` | `CCA_DEBUG_OFF` | `CCA_DEBUG_OFF` / `CCA_DEBUG_IN` / `CCA_DEBUG_OUT` / `CCA_DEBUG_ALL` |
| `DEVICE_PREFIX` | `"CCA-"` | Prefix für den Gerätenamen |
| `TCP_PORT` | `4210` | TCP-Port (nur WiFi) |
| `BAUD_RATE` | `115200` | Baudrate für den Seriellen Monitor |
| `HM10_RX_PIN` | `10` | RX-Pin des HM-10-Moduls (nur Arduino Uno/Nano) |
| `HM10_TX_PIN` | `11` | TX-Pin des HM-10-Moduls (nur Arduino Uno/Nano) |
| `HM10_BAUD` | `9600` | Baudrate des HM-10-Moduls (nur Arduino Uno/Nano) |
| `CCA_MAX_CALLBACKS` | `16` / `8` (AVR) | Max. Anzahl `onCommand()`-Handler |
| `CCA_MAX_RECEIVERS` | `16` / `8` (AVR) | Max. Anzahl `receive()`-Bindungen |
| `CCA_MAX_DISPLAY` | `16` / `8` (AVR) | Max. Anzahl gecachter `send()`-Werte |
| `CCA_MAX_COLOR` | `8` / `4` (AVR) | Max. Anzahl `receiveColor()`-Bindungen |

> **Kapazität erhöhen:** Wird die Standardgrenze überschritten, werden weitere Bindungen still ignoriert. Für komplexe Sketches vor `#include <CCARemote.h>` erhöhen:
> ```cpp
> #define CCA_MAX_RECEIVERS 24
> #define CCA_MAX_CALLBACKS 24
> #include <CCARemote.h>
> ```

---

## Elemente und Typen

### Steuerelemente (App → MCU)

| Element | Methode | Typ | Hinweis |
|---|---|---|---|
| Button | `receive()` | `bool` | `true` = gedrückt |
| Switch | `receive()` | `bool` | `true` = ein |
| Slider | `receive()` | `int` | Bereich in der App einstellbar (Standard 0–255) |
| Joystick | `receive()` | `int` | X und Y als separate Element-IDs |
| Input | `receive()` | `String` | Freier Text |
| Color Picker | `receiveColor()` | `int` | 3 Variablen: `r`, `g`, `b` (je 0–255) |
| Mode Selector | `receive()` | `int` | Ausgewählter Index (0–N); mit Option „Label senden": `String` (Label-Text) |

### Anzeigeelemente (MCU → App)

| Element | Methode | Hinweis |
|---|---|---|
| Display | `send()` | Messwert anzeigen |
| Gauge / Bar | `send()` | Balken / Kreisbogen |
| Chart | `send()` | Liniendiagramm |
| Status-LED | `send()` | Ganzzahl 0–3 |
| Label | `send()` | Text (optional, nur wenn Element-ID gesetzt) |

---

## API-Referenz

### `begin()` – Verbindung starten

Startet BLE-Advertising oder WLAN-Hotspot. Alle Parameter werden über die `#define`-Zeilen oben im Sketch gesetzt – `begin()` hat keine Parameter.

```cpp
void setup() {
  remote.begin();
  // ...
}
```

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

### `receiveColor()` – RGB-Farbwerte empfangen

Verknüpft ein Color-Picker-Element mit drei `int`-Variablen für Rot, Grün und Blau.

```cpp
int r = 0, g = 0, b = 0;

void setup() {
  remote.begin();
  remote.receiveColor("color1", r, g, b);  // Element-ID aus der App
}

void loop() {
  remote.handle();

  analogWrite(PIN_R, r);
  analogWrite(PIN_G, g);
  analogWrite(PIN_B, b);
}
```

---

### `receive()` – Mode Selector

Der Mode Selector sendet bei jeder Auswahl den **Index** der gewählten Option (`0`, `1`, `2`, …) als `int`. Mit der Option **„Label senden"** in der App wird stattdessen der Label-Text als `String` übermittelt.

```cpp
// Standard: Index-Modus
int betriebsModus = 0;

// Mit Option „Label senden" aktiv:
// String betriebsModus = "Aus";

void setup() {
  remote.begin();
  remote.receive("modus", betriebsModus);
}

void loop() {
  remote.handle();

  // Index-Modus (0 = Aus, 1 = Langsam, 2 = Schnell):
  switch (betriebsModus) {
    case 0: /* Aus     */ break;
    case 1: /* Langsam */ break;
    case 2: /* Schnell */ break;
  }

  // Label-Modus:
  // if (betriebsModus == "Langsam") { ... }
}
```

| Einstellung | Typ | Empfangener Wert |
|---|---|---|
| Standard (Index) | `int` | `0`, `1`, `2`, … |
| „Label senden" aktiv | `String` | `"Aus"`, `"Langsam"`, `"Schnell"`, … |

---

### Zustand bei Reconnect synchronisieren (`resync`)

Standardmäßig sendet die Library beim Verbinden nur Werte, die die MCU selbst via `send()`
verschickt hat. Werte, die die App an die MCU schickt (Slider, Switch, Color Picker …),
werden nach einem Reconnect **nicht** automatisch zurück zur App gesendet — die App würde
den Startwert aus dem Profil anzeigen, auch wenn die MCU einen anderen Wert hält.

Mit `resync = true` liest die Library beim Verbinden den **aktuellen Variablenwert** direkt
aus und sendet ihn an die App:

```cpp
int   speed = 0;
bool  power = false;
int   r = 255, g = 0, b = 0;

void setup() {
  remote.begin();
  remote.receive("speed",  speed,  true);         // resync bei Reconnect
  remote.receive("power",  power,  true);         // resync bei Reconnect
  remote.receiveColor("color1", r, g, b, true);   // resync bei Reconnect
}
```

Die App zeigt nach jedem Reconnect den tatsächlichen MCU-Zustand — egal ob der Wert von
der App gesetzt, intern geändert wurde oder nach einem Power Cycle auf den Sketch-Standardwert
zurückgefallen ist.

---

### Persistente Zustandsspeicherung

Variablenwerte werden automatisch gespeichert und beim nächsten Start wiederhergestellt — auch nach Neustart oder Stromverlust. Kein extra Code nötig:

```cpp
int  speed = 0;
bool power = false;

void setup() {
  remote.begin();
  remote.receive("speed", speed);  // wird automatisch gespeichert und geladen
  remote.receive("power", power);
}
```

| Plattform | Speicher | String-Persistenz |
|---|---|---|
| ESP32 | NVS (`Preferences`) | ✅ |
| ESP8266 | EEPROM (Flash-emuliert) | ❌ |
| AVR (Uno/Nano) | Hardware-EEPROM | ❌ |

**Deaktivieren:** Vor `#include <CCARemote.h>` einfügen:

```cpp
#define CCA_NO_PERSIST
#include <CCARemote.h>
```

**Letzten Zustand beim Start laden:** Standardmäßig werden die gespeicherten Werte erst beim ersten App-Connect in die Variablen geladen. Mit `loadState()` kann der Zustand bereits in `setup()` befüllt werden — ohne dass eine Verbindung nötig ist. Nützlich wenn der Controller ohne App sofort mit den letzten Einstellungen laufen soll (z. B. Licht, Effekt, Farbe):

```cpp
void setup() {
  remote.begin();

  remote.receive("switch1", switch1, true);
  remote.receive("slider1", slider1, true);
  remote.receiveColor("color1", r, g, b, true);

  remote.loadState();  // letzte Einstellungen sofort laden
                       // Ohne diese Zeile: Laden beim ersten App-Connect (bisheriges Verhalten)
}
```

> **Hinweis:** `loadState()` muss nach allen `receive()`-Aufrufen und vor der Hauptschleife aufgerufen werden. `loadState()` ist nicht verfügbar wenn `CCA_NO_PERSIST` gesetzt ist.

**Persistierten Zustand löschen:** Beim Wechsel zu einem Profil mit anderen Element-IDs können veraltete Einträge im NVS / EEPROM gelöscht werden:

```cpp
remote.clearState();  // löscht alle persistierten Werte
```

> **Hinweis:** `clearState()` ist nicht verfügbar wenn `CCA_NO_PERSIST` gesetzt ist.

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
| `send(String)` | Rohformat `"key:value"` – für fortgeschrittene Anwendungsfälle |

> **Hinweis – Label-Element:** Neben Display-, Gauge-, Chart- und LED-Elementen kann auch das **Label**-Element Werte empfangen. `remote.send("label1", "Text")` aktualisiert den angezeigten Text des Labels dynamisch. Die Element-ID muss dazu im Label-Editor der App eingetragen sein.

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

### `watchdog()` – Automatischer Nullwert bei Verbindungsverlust

Setzt eine Variable automatisch auf `0` zurück wenn sie länger als das angegebene Timeout nicht aktualisiert wurde. Typischer Anwendungsfall: Joystick-Achsen bei RC-Fahrzeugen oder Robotern, damit das Gerät bei Verbindungsverlust zuverlässig stoppt.

```cpp
void setup() {
  remote.begin();

  remote.receive("axisX", axisX);
  remote.receive("axisY", axisY);
  remote.watchdog("axisX", 500);  // axisX → 0 wenn 500 ms kein Update
  remote.watchdog("axisY", 500);  // axisY → 0 wenn 500 ms kein Update
}
```

| Parameter | Typ | Beschreibung |
|---|---|---|
| `cmd` | `String` | Element-ID — muss mit `receive()` registriert sein |
| `timeoutMs` | `unsigned long` | Timeout in Millisekunden |

> **Hinweis:** `watchdog()` muss nach `receive()` aufgerufen werden. Der Watchdog wird in `handle()` geprüft — `handle()` muss also regelmäßig in `loop()` aufgerufen werden.

---

### `debug()` – Debug-Level zur Laufzeit ändern

Normalerweise wird der Debug-Level über `#define DEBUG_LEVEL` festgelegt. Mit `debug()` kann er im Sketch nachträglich geändert werden:

```cpp
remote.debug(CCA_DEBUG_ALL);    // IN + OUT ausgeben
remote.debug(CCA_DEBUG_IN);     // nur empfangene Werte
remote.debug(CCA_DEBUG_OUT);    // nur gesendete Werte
remote.debug(CCA_DEBUG_OFF);    // Debug deaktivieren
```

| Modus | Wert | Beschreibung |
|---|---|---|
| `CCA_DEBUG_OFF` | `0` | Kein Debug-Output |
| `CCA_DEBUG_IN`  | `1` | Empfangene Werte ausgeben (`[CCA] IN  key = wert`) |
| `CCA_DEBUG_OUT` | `2` | Gesendete Werte ausgeben (`[CCA] OUT key = wert`) |
| `CCA_DEBUG_ALL` | `3` | Empfangene und gesendete Werte ausgeben |

---

### Gerätename und Prefix anpassen

```cpp
#define DEVICE_NAME   "Roboter"
#define DEVICE_PREFIX "HTL-"    // → "HTL-Roboter"
// oder:
#define DEVICE_PREFIX ""        // → "Roboter"  (kein Prefix)
```

---

## Vollständiges Beispiel

```cpp
// ---- Konfiguration – hier anpassen! -----------------------
#define DEVICE_NAME  "MeinName"
#define CONNECTION   CCA_BLE
#define PASSWORD     ""
#define DEBUG_LEVEL  CCA_DEBUG_ALL
// -----------------------------------------------------------

#include <CCARemote.h>

const int LED_BUTTON =  2;  // ESP32-C3: GPIO 18/19 sind USB-Pins – bitte meiden
const int LED_SLIDER =  4;  // PWM-fähig
const int LED_SWITCH =  5;

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

## Color Picker – RGB-LED Beispiel

```cpp
// ---- Konfiguration – hier anpassen! -----------------------
#define DEVICE_NAME  "MeinName"
#define CONNECTION   CCA_BLE
#define PASSWORD     ""
#define DEBUG_LEVEL  CCA_DEBUG_ALL
// -----------------------------------------------------------

#include <CCARemote.h>

// Pins der gemeinsamen Kathode RGB-LED (PWM-fähige Pins)
const int PIN_R = 25;
const int PIN_G = 26;
const int PIN_B = 27;

int r = 0, g = 0, b = 0;

void setup() {
  pinMode(PIN_R, OUTPUT);
  pinMode(PIN_G, OUTPUT);
  pinMode(PIN_B, OUTPUT);

  remote.begin();
  remote.receiveColor("color1", r, g, b);  // Element-ID aus der App
}

void loop() {
  remote.handle();

  if (remote.isConnected()) {
    analogWrite(PIN_R, r);
    analogWrite(PIN_G, g);
    analogWrite(PIN_B, b);
  }
}
```

> **Hinweis:** Bei einer gemeinsamen Anode RGB-LED die Werte invertieren: `analogWrite(PIN_R, 255 - r)` usw.

---

## Arduino Uno / Nano mit HM-10-Modul

Für Arduino Uno und Nano wird `CONNECTION CCA_BLE` automatisch über ein HM-10-Modul per SoftwareSerial abgewickelt. Die Pin-Konfiguration erfolgt ebenfalls über `#define`:

```cpp
// ---- Konfiguration – hier anpassen! -----------------------
#define DEVICE_NAME  "MeinName"
#define CONNECTION   CCA_BLE
#define PASSWORD     ""
#define DEBUG_LEVEL  CCA_DEBUG_ALL
#define HM10_RX_PIN  10          // Standard: 10
#define HM10_TX_PIN  11          // Standard: 11
#define HM10_BAUD    9600        // Standard: 9600
// -----------------------------------------------------------

#include <CCARemote.h>

bool ledAn = false;

void setup() {
  remote.begin();
  remote.receive("ledAn", ledAn);
  pinMode(LED_BUILTIN, OUTPUT);
}

void loop() {
  remote.handle();
  digitalWrite(LED_BUILTIN, ledAn ? HIGH : LOW);
}
```

### HM-10 Verdrahtung

| HM-10 | Arduino Uno / Nano |
|---|---|
| VCC | 3,3 V oder 5 V (je nach Modul) |
| GND | GND |
| TXD | Pin 10 (RX, via `HM10_RX_PIN`) |
| RXD | Pin 11 (TX, via `HM10_TX_PIN`) – bei 5-V-Arduinos Spannungsteiler empfohlen¹ |

> ¹ Der HM-10 arbeitet mit 3,3-V-Logik. Ein Spannungsteiler (z. B. 10 kΩ / 20 kΩ) schützt den Eingang des Moduls vor den 5-V-Pegeln des Arduino.

> **Hinweis für Klon-Module:** Viele günstige HM-10 Klon-Module haben TXD und RXD aus der Gegenperspektive beschriftet. Falls keine Verbindung zustande kommt, einfach die beiden Datenleitungen tauschen.

### Remote-Profil auf Uno / Nano (`setProfile`)

Der Arduino Uno und Nano verfügen nur über **2 KB SRAM**. Ein Remote-Profil mit wenigen Elementen erzeugt bereits einen Konfig-String von 300–600 Bytes. Wird dieser String direkt im RAM abgelegt, kann das den verfügbaren Speicher erschöpfen und zu sekündlichen Resets führen – der Arduino IDE-Compiler warnt dann mit:

> *Wenig Arbeitsspeicher verfügbar, es können Stabilitätsprobleme auftreten.*

**Lösung:** Den Profil-String mit `PROGMEM` im Flash (32 KB) ablegen und den passenden Cast verwenden:

```cpp
// Richtig: PROGMEM – belegt kein RAM
const char PROFILE[] PROGMEM =
  "v:2"
  "|nm:MeinProfil"
  "|sl:speed:0:255@0,0,200,60,0,0,200,60"
  "|sw:enable@0,70,120,50,0,70,120,50";

void setup() {
  remote.begin();
  remote.setProfile((const __FlashStringHelper*)PROFILE);
}
```

Alternativ funktioniert auch das `F()`-Makro direkt im Aufruf:

```cpp
remote.setProfile(F("v:2|nm:MeinProfil|sl:speed:0:255@0,0,200,60,0,0,200,60"));
```

> **Wichtig:** Den Profil-String **niemals** aus einer lokalen Variable oder einem `String`-Objekt übergeben – der Pointer muss für die gesamte Laufzeit gültig bleiben. Globale `PROGMEM`-Konstanten und String-Literale (via `F()`) erfüllen diese Bedingung.

> **ESP32 / ESP8266:** Auf diesen Plattformen sind `PROGMEM` und der Cast nicht erforderlich – `remote.setProfile(PROFILE)` funktioniert weiterhin wie gewohnt.

---

### Weitere HM-10 Hinweise

- **BLE-Name:** Der Gerätename muss bei einigen Klon-Modellen einmalig manuell per AT-Befehl gesetzt werden: `AT+NAMEMeinName` (kein Leerzeichen, kein Zeilenumbruch). Der Name bleibt dauerhaft im Flash gespeichert.
- Bei Auth-Fehler kann das HM-10 die Verbindung nicht aktiv trennen; die App übernimmt das und zeigt die Fehlermeldung an.
- Gerätenamen werden auf 12 Zeichen begrenzt (HM-10-Firmware-Limit).
- Getestete Firmware: v5xx.

---

## Voraussetzungen

### ESP32

- **Board:** ESP32 (beliebiges Modell)
- **Arduino IDE:** 2.x empfohlen
- **ESP32-Paket:** Boardverwalter → `esp32` von Espressif

### ESP8266

- **Board:** ESP8266 (beliebiges Modell)
- **Arduino IDE:** 2.x empfohlen
- **ESP8266-Paket:** Boardverwalter → `esp8266` von Community

### Arduino Uno / Nano + HM-10

- **Board:** Arduino Uno oder Nano (ATmega328P)
- **Arduino IDE:** 2.x empfohlen
- **Bibliothek:** `SoftwareSerial` (im Arduino IDE vorinstalliert)
- **Modul:** HM-10 BLE-Modul (CC2540 / CC2541 Chip, Firmware v5xx)

---

## Hinweis
Diese Bibliothek basiert auf der Diplomarbeit von L. Eder und E. Duyar im Rahmen ihrer Ausbildung an der HTL Anichstraße. Die vorliegende Version wurde von A. Eckhart mit freundlicher Genehmigung der ursprünglichen Autoren erweitert und veröffentlicht. Die Nutzung erfolgt auf eigene Verantwortung – es wird keine Gewährleistung für Richtigkeit, Vollständigkeit oder Eignung für einen bestimmten Zweck übernommen.
