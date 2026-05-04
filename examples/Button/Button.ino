// ============================================================
//  CCA Remote Beispiel: Switch Element
//  Ein Taster in der App schaltet eine LED ein oder aus.
// ============================================================
//  Erfordert die kostenlose CCA Remote App (Android / iOS)
// ============================================================

// Für BLE Verbindung
#include <CCARemoteBLE.h>
CCARemoteBLE remote("MeinName");  // Namen hier anpassen!

// Für WiFi Verbindung
// #include <CCARemoteWiFi.h>
// CCARemoteWiFi remote("MeinName");  // Namen hier anpassen!

const int LED_PIN = 18;  // LED Pin

// --- Variablen - speichern von App übermittelte Werte ---
// --- werden von remote.handle() automatisch aktualisiert ---
bool button_status = false;  // true = aktiv; false = inaktiv


void setup() {
  remote.debug(true, 9600);  // Debug Modus aktivieren – vor begin() aufrufen!
  remote.begin();            // Initialisierung (erforderlich)

  // Die Element-IDs aus der App werden nun mit den vorhin definierten Variablen verknüpft
  // remote.receive("Element-ID", variable);
  remote.receive("button1", button_status);

  pinMode(LED_PIN, OUTPUT);
}

void loop() {
  remote.handle();  // Empfangene Befehle verarbeiten und Variablen aktualisieren (erforderlich)

  if (remote.isConnected()) {
    digitalWrite(LED_PIN, button_status);
  } else {
    digitalWrite(LED_PIN, LOW);  // LED ausschalten wenn keine Verbindung
  }
}
