// ============================================================
//  CCA Remote Beispiel: Button Element
//  Ein Taster in der App schaltet eine LED ein oder aus.
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

const int LED_PIN = 18;  // LED Pin

// --- Variablen - speichern von App übermittelte Werte ---
// --- werden von remote.handle() automatisch aktualisiert ---
bool button_status = false;  // true = gedrückt; false = losgelassen


void setup() {
  remote.begin();

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
