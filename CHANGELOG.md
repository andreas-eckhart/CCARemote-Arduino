# Changelog

Alle wesentlichen Änderungen werden in dieser Datei dokumentiert.
Format orientiert sich an [Keep a Changelog](https://keepachangelog.com/de/1.0.0/).

---

## [1.2.1] – 2026-06-04

### Neu
- **Persistente Zustandsspeicherung:** Variablenwerte (`receive()`, `receiveColor()`) werden automatisch gespeichert und nach einem Neustart oder Stromverlust wiederhergestellt.
  - **ESP32:** NVS via `Preferences`-Bibliothek (kein `commit()` nötig)
  - **ESP8266:** EEPROM (flash-emuliert) mit `EEPROM.begin()` / `EEPROM.commit()`
  - **AVR (Uno/Nano):** Hardware-EEPROM mit `EEPROM.update()` (schreibschonend)
  - Mit `#define CCA_NO_PERSIST` vor dem `#include` vollständig deaktivierbar
  - `_loadState()` wird automatisch in `begin()` aufgerufen – kein Benutzercode nötig
  - `_saveState()` wird nach jeder Wertänderung in `processCommand()` aufgerufen
- **Vollständiger Resync beim Connect:** `_resyncDisplay()` sendet nun alle registrierten Variablenwerte an die App – nicht mehr nur Variablen mit `resync=true`.
- **`clearState()`:** Löscht alle persistierten Werte (NVS-Namespace / EEPROM-Magic). Nützlich beim Wechsel zu einem Profil mit anderen Element-IDs, um veraltete Einträge zu entfernen.

### Technische Details
- EEPROM-Layout (AVR/ESP8266): `[magic(1)] + [type(1)+value(4)] × CCA_MAX_RECEIVERS + [r+g+b als int] × CCA_MAX_COLOR`
- ESP32: Keys werden mit Suffix `_r`, `_g`, `_b` für Farben in NVS gespeichert
- String-Variablen (`STRING_T`) werden auf AVR und ESP8266 nicht persistiert (Heap-Limitation)

---

## [1.2.0] – 2025-05-27

### Neu
- Profil-Konfiguration: `setProfile()` – Profil-String wird beim Verbindungsaufbau an die App übertragen
- `resync=true` Parameter in `receive()` und `receiveColor()` – Wert bei Reconnect zur App senden
- `sendAlways()` – Wert auch bei Gleichheit senden (für Chart-Elemente)
- Authentifizierung per Passwort (BLE und WiFi)
- Watchdog-Mechanismus: Variable automatisch auf 0 setzen bei Verbindungsabbruch
- Mode Selector: Index- und Label-Modus
- Werte von Label, Slider- und Switch-Elementen können nun vom Controller an die App übergeben werden

---

## [1.1.0] – 2025-05-21

### Neu
- Connection Manager: erlaubt das Speichern verschiedener Verbindungsprofile
- Color Picker hinzugefügt
- Watchdog Funktion: Wenn Joystick in definierter Zeitspanne keinen Wert sendet wird automatisch für X- und y-Achse der Wert 0 gesetzt

### Geändert
- WiFi Übertragungsgeschwindigkeit verbessert
- vereifachte Library Konfiguration
- Protokoll Versionierung hinzugefügt


---

## [1.0.0] – 2025-05-11

### Erstveröffentlichung
- BLE-Verbindung (ESP32 nativ, AVR via HM-10 Modul)
- WiFi-Verbindung (Access Point + TCP, ESP32 und ESP8266)
- `receive()` für `int`, `bool`, `float`, `String`
- `receiveColor()` / `getColor()` für RGB-Color-Picker
- `send()` für Display-Elemente
- `watchdog()` für Joystick-Achsen
- `on_command()` – Callback-Registrierung