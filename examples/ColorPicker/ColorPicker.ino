// ============================================================
//  CCA Remote Beispiel: Color Picker
//  Steuert eine RGB-LED mit dem Color-Picker-Element der App.
//  PWM-fähige Pins erforderlich.
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

// PWM-fähige Pins für die RGB-LED (gemeinsame Kathode)
const int PIN_R = 25;
const int PIN_G = 26;
const int PIN_B = 27;

// Farbvariablen – werden von remote.handle() automatisch aktualisiert
int r = 0;
int g = 0;
int b = 0;


void setup() {
  remote.begin();

  // Element-ID "color1" aus der App mit den drei Farbvariablen verknüpfen
  remote.receiveColor("color1", r, g, b);

  pinMode(PIN_R, OUTPUT);
  pinMode(PIN_G, OUTPUT);
  pinMode(PIN_B, OUTPUT);
}

void loop() {
  remote.handle();  // Empfangene Befehle verarbeiten (erforderlich!)

  if (remote.isConnected()) {
    analogWrite(PIN_R, r);
    analogWrite(PIN_G, g);
    analogWrite(PIN_B, b);
  } else {
    analogWrite(PIN_R, 0);
    analogWrite(PIN_G, 0);
    analogWrite(PIN_B, 0);
  }
}
