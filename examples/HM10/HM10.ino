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

#include <CCARemoteBLE.h>

// Standard-Pins: RX=10 (→ HM-10 TXD), TX=11 (→ HM-10 RXD)
CCARemoteBLE remote("MeinName");  // Namen hier anpassen!

// Eigene Pins oder Baudrate:
// CCARemoteBLE remote("MeinName", "CCA-", 10, 11, 9600);

const int LED_PIN = 13;  // interne LED am Arduino Uno / Nano

bool button1_val = false;
bool switch1_val = false;

void setup() {
  remote.debug(CCA_DEBUG_ALL, 9600);
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
