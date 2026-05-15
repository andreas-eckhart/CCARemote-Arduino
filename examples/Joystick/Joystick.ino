// ============================================================
//  CCA Remote Beispiel: Joystick Element mit Watchdog
//  Ein Joystick in der App steuert zwei PWM-Ausgänge (z.B.
//  Motoren eines RC-Fahrzeugs). Der Watchdog stoppt die Motoren
//  automatisch wenn die Verbindung unterbrochen wird.
// ============================================================
//  Erfordert die kostenlose CCA Remote App (Android / iOS)
// ============================================================

// Für BLE Verbindung
#include <CCARemoteBLE.h>
CCARemoteBLE remote("MeinName");  // Namen hier anpassen!

// Für WiFi Verbindung
// #include <CCARemoteWiFi.h>
// CCARemoteWiFi remote("MeinName");  // Namen hier anpassen!

const int PIN_MOTOR_A = 18;  // PWM-Pin Motor A (z.B. Vorwärts / Rückwärts)
const int PIN_MOTOR_B = 19;  // PWM-Pin Motor B (z.B. Links / Rechts)

// --- Variablen - speichern von App übermittelte Werte ---
// --- werden von remote.handle() automatisch aktualisiert ---
int axisX = 0;  // Joystick X  (-255 bis +255)
int axisY = 0;  // Joystick Y  (-255 bis +255)


void setup() {
  remote.debug(CCA_DEBUG_ALL, 9600);  // Debug Modus: CCA_DEBUG_IN, CCA_DEBUG_OUT oder CCA_DEBUG_ALL
  remote.begin();                     // Initialisierung (erforderlich)

  // Die Element-IDs aus der App werden nun mit den vorhin definierten Variablen verknüpft
  // remote.receive("Element-ID", variable);
  remote.receive("axisX", axisX);
  remote.receive("axisY", axisY);

  // Watchdog: axisX und axisY werden auf 0 gesetzt wenn die App
  // länger als 500 ms keine Werte sendet (z.B. bei Verbindungsverlust).
  // So bleibt das Fahrzeug zuverlässig stehen wenn der Finger losgelassen
  // wird oder die Verbindung abbricht.
  remote.watchdog("axisX", 500);
  remote.watchdog("axisY", 500);

  pinMode(PIN_MOTOR_A, OUTPUT);
  pinMode(PIN_MOTOR_B, OUTPUT);
}

void loop() {
  remote.handle();  // Empfangene Befehle verarbeiten und Variablen aktualisieren (erforderlich)

  if (remote.isConnected()) {
    // axisX / axisY: -255 bis +255 → analogWrite erwartet 0 bis 255
    // Hier wird der Absolutwert verwendet; das Vorzeichen bestimmt die Richtung.
    analogWrite(PIN_MOTOR_A, abs(axisY));  // Throttle
    analogWrite(PIN_MOTOR_B, abs(axisX));  // Lenkung
  } else {
    analogWrite(PIN_MOTOR_A, 0);
    analogWrite(PIN_MOTOR_B, 0);
  }
}
