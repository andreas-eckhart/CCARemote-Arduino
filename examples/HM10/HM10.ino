// ============================================================
//  CCA Remote Beispiel: Arduino Uno / Nano mit HM-10 Modul
//  Ein Button und ein Switch in der App steuern die interne LED.
// ============================================================
//  Erfordert die kostenlose CCA Remote App (Android / iOS)
// ============================================================
//
//  HM-10 Verdrahtung:
//    HM-10 VCC  →  3,3 V (oder 5 V je nach Modul)
//    HM-10 GND  →  GND
//    HM-10 TXD  →  Pin 10  (Arduino RX)
//    HM-10 RXD  →  Pin 11  (Arduino TX)
//
//  Hinweis: Manche Klon-Module haben TXD/RXD vertauscht beschriftet.
//           Falls keine Verbindung zustande kommt, Pin 10 und Pin 11 tauschen.
// ============================================================

// ---- Konfiguration – hier anpassen! -----------------------
#define DEVICE_NAME  "MeinName"    // Gerätename
#define CONNECTION   CCA_BLE       // muss CCA_BLE sein (kein WiFi am UNO)
#define PASSWORD     ""            // BLE-AUTH Passwort (leer = ohne)
#define DEBUG_LEVEL  CCA_DEBUG_ALL // CCA_DEBUG_OFF / _IN / _OUT / _ALL

// Optional – nur setzen wenn Standardwert nicht passt:
// #define DEVICE_PREFIX "XYZ-"   // Standard: "CCA-"
// #define HM10_RX_PIN   10       // Standard: 10  (Arduino-Pin → HM-10 TXD)
// #define HM10_TX_PIN   11       // Standard: 11  (Arduino-Pin → HM-10 RXD)
// #define HM10_BAUD     9600     // Standard: 9600
// #define BAUD_RATE     9600     // Standard: 115200 (Serial Monitor)
// -----------------------------------------------------------

#include <CCARemote.h>

const int LED_PIN = 13;  // interne LED am Arduino Uno / Nano

bool button1_val = false;
bool switch1_val = false;


void setup() {
  remote.begin();

  remote.receive("button1", button1_val);
  remote.receive("switch1", switch1_val);

  pinMode(LED_PIN, OUTPUT);
}

void loop() {
  remote.handle();

  if (remote.isConnected()) {
    digitalWrite(LED_PIN, button1_val || switch1_val);
  } else {
    digitalWrite(LED_PIN, LOW);
  }
}
