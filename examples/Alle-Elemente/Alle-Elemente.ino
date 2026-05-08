// ============================================================
//  CCA Remote Beispiel: alle Elemente
//  Alle unterstützten Steuerelemente der App. 
//  Empfangene Werte werden im seriellen Monitor ausgegeben.
// ============================================================
//  Erfordert die kostenlose CCA Remote App (Android / iOS)
// ============================================================

// Für BLE Verbindung
#include <CCARemoteBLE.h>
CCARemoteBLE remote("MeinName");  // Namen hier anpassen!

// Für WiFi Verbindung
// #include <CCARemoteWiFi.h>
// CCARemoteWiFi remote("MeinName");  // Namen hier anpassen!

const int LED_BUTTON = 25;  // LED für Button-Element
const int LED_SLIDER = 18;  // LED für Slider-Element (PWM-fähiger Pin)
const int LED_SWITCH = 21;  // LED für Switch-Element
const int LED_INPUT  = 22;  // LED für Input-Element

// --- Variablen - speichern von App übermittelte Werte ---
// --- werden von remote.handle() automatisch aktualisiert ---
bool   button1_val = false;  // true = gedrückt; false = nicht gedrückt
int    slider1_val = 0;      // 0 bis 255 (Slider-Bereich in der App anpassbar)
bool   switch1_val = false;  // true = aktiv; false = inaktiv
int    axisX_val   = 0;      // X-Achse: -255 bis +255 (Joystick-Bereich in der App anpassbar)
int    axisY_val   = 0;      // Y-Achse: -255 bis +255 (Joystick-Bereich in der App anpassbar)
String command     = "";     // beliebige Benutzereingabe


void setup() {
  remote.debug(CCA_DEBUG_ALL, 9600);  // Debug Modus: CCA_DEBUG_IN, CCA_DEBUG_OUT oder CCA_DEBUG_ALL
  remote.begin();            // Initialisierung (erforderlich)

  // Die Element-IDs aus der App werden nun mit den vorhin definierten Variablen verknüpft
  // remote.receive("Element-ID", variable);
  remote.receive("button1", button1_val);
  remote.receive("slider1", slider1_val);
  remote.receive("switch1", switch1_val);
  remote.receive("input1",  command);
  remote.receive("axisX",   axisX_val);
  remote.receive("axisY",   axisY_val);

  pinMode(LED_BUTTON, OUTPUT);
  pinMode(LED_SLIDER, OUTPUT);
  pinMode(LED_SWITCH, OUTPUT);
  pinMode(LED_INPUT,  OUTPUT);
}

void loop() {
  remote.handle();  // Empfangene Befehle verarbeiten und Variablen aktualisieren (erforderlich)

  if (remote.isConnected()) {
    // LED mit Button-Element schalten
    digitalWrite(LED_BUTTON, button1_val);

    // LED mit Slider-Element dimmen
    analogWrite(LED_SLIDER, slider1_val);

    // LED mit Switch-Element schalten
    digitalWrite(LED_SWITCH, switch1_val);

    // LED mit Input-Element schalten: "ON" = an, alles andere = aus
    if (command.length() > 0) {
      digitalWrite(LED_INPUT, command == "ON" ? HIGH : LOW);
    }

    // Slider-Wert an Display-Elemente der App senden
    remote.send("display1", slider1_val);

    // Hardware-Typ (ESP32) an Label-Element der App senden
    remote.send("label1", "ESP32");

  } else {
    // Alle Ausgaben auf sicheren Zustand setzen wenn keine Verbindung
    digitalWrite(LED_BUTTON, LOW);
    analogWrite(LED_SLIDER, 0);
    digitalWrite(LED_SWITCH, LOW);
    digitalWrite(LED_INPUT,  LOW);
  }
}
