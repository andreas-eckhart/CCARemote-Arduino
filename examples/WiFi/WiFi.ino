// ============================================================
//  CCA Remote Beispiel: WiFi Verbindung
//  Ein Schalter in der App schaltet eine LED ein oder aus.
// ============================================================
//  Erfordert die kostenlose CCA Remote App (Android / iOS)
// ============================================================

// Für BLE Verbindung
// #include <CCARemoteBLE.h>
// CCARemoteBLE remote("MeinName");  // Namen hier anpassen!

// Für WiFi Verbindung
#include <CCARemoteWiFi.h>
CCARemoteWiFi remote("MeinName");  // Namen hier anpassen!

const int LED_PIN = 18;  // LED Pin

// --- Variablen - speichern von App übermittelte Werte ---
// --- werden von remote.handle() automatisch aktualisiert ---
bool switch_led = false;  // true = aktiv; false = inaktiv


void setup() {
  remote.debug(true, 9600);  // Debug Modus aktivieren – vor begin() aufrufen!
  remote.begin("geheim1234"); // WLAN-Hotspot-Passwort festlegen (leer ist offener AP)

  // Die Element-IDs aus der App werden nun mit den vorhin definierten Variablen verknüpft
  // remote.receive("Element-ID", variable);
  remote.receive("switch1", switch_led);

  pinMode(LED_PIN, OUTPUT);
}

void loop() {
  remote.handle();  // Empfangene Befehle verarbeiten und Variablen aktualisieren (erforderlich)

  digitalWrite(LED_PIN, switch_led ? HIGH : LOW);
  
}
