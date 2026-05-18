// ============================================================
//  CCA Remote Beispiel: Alle Elemente
//  Alle unterstützten Steuerelemente der App.
//  Empfangene Werte werden im seriellen Monitor ausgegeben.
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

const int LED_BUTTON = 25;  // LED für Button-Element
const int LED_SLIDER = 18;  // LED für Slider-Element (PWM-fähiger Pin)
const int LED_SWITCH = 21;  // LED für Switch-Element
const int LED_INPUT  = 22;  // LED für Input-Element
const int PIN_R      = 26;  // PWM-Pin für RGB-LED Rot   (Color-Picker)
const int PIN_G      = 27;  // PWM-Pin für RGB-LED Grün  (Color-Picker)
const int PIN_B      = 32;  // PWM-Pin für RGB-LED Blau  (Color-Picker)

// --- Variablen - speichern von App übermittelte Werte ---
// --- werden von remote.handle() automatisch aktualisiert ---
bool   button1_val = false;  // true = gedrückt; false = nicht gedrückt
int    slider1_val = 0;      // 0 bis 255 (Slider-Bereich in der App anpassbar)
bool   switch1_val = false;  // true = ein; false = aus
int    axisX_val   = 0;      // X-Achse: -255 bis +255
int    axisY_val   = 0;      // Y-Achse: -255 bis +255
String command     = "";     // beliebige Benutzereingabe
int    r = 0, g = 0, b = 0;  // RGB-Farbwerte (Color-Picker)


void setup() {
  remote.begin();

  // Die Element-IDs aus der App werden nun mit den vorhin definierten Variablen verknüpft
  // remote.receive("Element-ID", variable);
  remote.receive("button1", button1_val);
  remote.receive("slider1", slider1_val);
  remote.receive("switch1", switch1_val);
  remote.receive("input1",  command);
  remote.receive("axisX",   axisX_val);
  remote.receive("axisY",   axisY_val);
  remote.receiveColor("color1", r, g, b);

  pinMode(LED_BUTTON, OUTPUT);
  pinMode(LED_SLIDER, OUTPUT);
  pinMode(LED_SWITCH, OUTPUT);
  pinMode(LED_INPUT,  OUTPUT);
  pinMode(PIN_R, OUTPUT);
  pinMode(PIN_G, OUTPUT);
  pinMode(PIN_B, OUTPUT);
}

void loop() {
  remote.handle();  // Empfangene Befehle verarbeiten und Variablen aktualisieren (erforderlich)

  if (remote.isConnected()) {
    digitalWrite(LED_BUTTON, button1_val);
    analogWrite(LED_SLIDER,  slider1_val);
    digitalWrite(LED_SWITCH, switch1_val);

    // LED mit Input-Element schalten: "ON" = an, alles andere = aus
    if (command.length() > 0) {
      digitalWrite(LED_INPUT, command == "ON" ? HIGH : LOW);
    }

    // Color-Picker-Werte auf RGB-LED ausgeben
    analogWrite(PIN_R, r);
    analogWrite(PIN_G, g);
    analogWrite(PIN_B, b);

    // Slider-Wert an Display-Element der App senden
    remote.send("display1", slider1_val);

    // Hardware-Typ an Label-Element der App senden
    remote.send("label1", "ESP32");

  } else {
    // Alle Ausgaben auf sicheren Zustand setzen wenn keine Verbindung
    digitalWrite(LED_BUTTON, LOW);
    analogWrite(LED_SLIDER,  0);
    digitalWrite(LED_SWITCH, LOW);
    digitalWrite(LED_INPUT,  LOW);
    analogWrite(PIN_R, 0);
    analogWrite(PIN_G, 0);
    analogWrite(PIN_B, 0);
  }
}
