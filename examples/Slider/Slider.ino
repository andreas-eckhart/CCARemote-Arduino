// ============================================================
//  CCA Remote Beispiel: Slider Element
//  Ein Slider in der App steuert die Helligkeit einer LED.
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
int helligkeit = 0;   // 0 bis 255 (Slider-Bereich in der App anpassbar)


void setup() {
  remote.debug(CCA_DEBUG_ALL, 9600);  // Debug Modus: CCA_DEBUG_IN, CCA_DEBUG_OUT oder CCA_DEBUG_ALL
  remote.begin();            // Initialisierung (erforderlich)

  // Die Element-IDs aus der App werden nun mit den vorhin definierten Variablen verknüpft
  // remote.receive("Element-ID", variable);
  remote.receive("slider1", helligkeit);

  pinMode(LED_PIN, OUTPUT);
}

void loop() {
  remote.handle();  // Empfangene Befehle verarbeiten und Variablen aktualisieren (erforderlich)

  if (remote.isConnected()) {
    analogWrite(LED_PIN, helligkeit);
  } else {
    analogWrite(LED_PIN, 0);  // LED ausschalten wenn keine Verbindung
  }
}
