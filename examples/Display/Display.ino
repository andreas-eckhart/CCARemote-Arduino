// ============================================================
//  CCA Remote Beispiel: Display Element
//  Aktualisiert jede Sekunde einen Zähler und gibt den Wert
//  in einem Display-Element in der App aus.
// ============================================================
//  Erfordert die kostenlose CCA Remote App (Android / iOS)
// ============================================================

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

unsigned long lastUpdate = 0;
const unsigned long UPDATE_INTERVAL = 1000;

int counter = 0;


void setup() {
  remote.begin();
}

void loop() {
  remote.handle();  // Empfangene Befehle verarbeiten und Variablen aktualisieren (erforderlich)

  // Wert jede Sekunde senden (nur wenn App verbunden)
  if (remote.isConnected() && millis() - lastUpdate >= UPDATE_INTERVAL) {
    lastUpdate = millis();
    counter++;
    remote.send("display1", counter);
  }
}
