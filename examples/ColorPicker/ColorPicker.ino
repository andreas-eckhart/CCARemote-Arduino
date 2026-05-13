// ============================================================
//  CCA Remote Beispiel: Color Picker
//  Steuert eine RGB-LED (oder einen LED-Streifen) mit dem
//  Color-Picker-Element der CCA Remote App.
//
//  Wert-Format der App:  R;G;B  (z.B. "255;128;0")
//  Die drei Variablen r, g, b werden automatisch aktualisiert.
// ============================================================
//  Erfordert die kostenlose CCA Remote App (Android / iOS)
//  Getestet auf ESP32  |  PWM-fähige Pins erforderlich
// ============================================================

// Für BLE-Verbindung:
#include <CCARemoteBLE.h>
CCARemoteBLE remote("MeinName");  // Namen hier anpassen!

// Für WiFi-Verbindung stattdessen:
// #include <CCARemoteWiFi.h>
// CCARemoteWiFi remote("MeinName");

// PWM-fähige Pins für die RGB-LED (gemeinsame Kathode)
const int PIN_R = 25;
const int PIN_G = 26;
const int PIN_B = 27;

// Farbvariablen – werden von remote.handle() automatisch aktualisiert
int r = 0;
int g = 0;
int b = 0;


void setup() {
  remote.debug(CCA_DEBUG_ALL, 9600);
  remote.begin();

  // Element-ID "color1" aus der App mit den drei Farbvariablen verknüpfen
  // Die Element-ID muss mit der ID in der App übereinstimmen
  remote.receiveColor("color1", r, g, b);

  pinMode(PIN_R, OUTPUT);
  pinMode(PIN_G, OUTPUT);
  pinMode(PIN_B, OUTPUT);
}


void loop() {
  remote.handle();  // Empfangene Befehle verarbeiten (erforderlich!)

  if (remote.isConnected()) {
    // RGB-LED mit den empfangenen Farbwerten ansteuern
    analogWrite(PIN_R, r);
    analogWrite(PIN_G, g);
    analogWrite(PIN_B, b);
  } else {
    // LED ausschalten wenn keine Verbindung
    analogWrite(PIN_R, 0);
    analogWrite(PIN_G, 0);
    analogWrite(PIN_B, 0);
  }
}
