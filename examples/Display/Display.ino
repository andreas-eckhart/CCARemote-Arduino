// ============================================================
//  CCA Remote Beispiel: Display Element
//  Aktualisiert jede Sekunde einen Zähler und gibt den Wert
//  in einem Display Element in der App aus.
// ============================================================
//  Erfordert die kostenlose CCA Remote App (Android / iOS)
// ============================================================

// Für BLE Verbindung
#include <CCARemoteBLE.h>
CCARemoteBLE remote("MeinName");  // Namen hier anpassen!

// Für WiFi Verbindung
// #include <CCARemoteWiFi.h>
// CCARemoteWiFi remote("MeinName");  // Namen hier anpassen!

unsigned long lastUpdate = 0;
const unsigned long UPDATE_INTERVAL = 1000;

int counter = 0;


void setup() {
  remote.begin();            // Initialisierung (erforderlich)
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
