// ============================================================
//  CCA Remote Beispiel: Remote Profil
//  Das Profil-Layout wird direkt im Code eingebettet –
//  die App erstellt es beim ersten Verbinden automatisch.
//  Ein Slider steuert die LED-Helligkeit, ein Display
//  zeigt den aktuellen Prozentwert an.
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

const int LED_PIN = 2;  // PWM-fähiger Pin (ESP32); ggf. anpassen

// Profil-String – erzeugt in der App unter Profil → "Profil-Code kopieren" → Plattform wählen.
// Tipp: Auf AVR (Uno/Nano) stattdessen F("...") verwenden um Flash statt RAM zu nutzen (siehe README).
const char PROFILE[] =
    "v:3"
    "|nm:LED Demo"
    "|lb:titel:LED Helligkeit:fs=22:b=1:c=FF2196F3"
    "@0.0000,0.0000,400.0000,44.0000,0.0000,0.0000,400.0000,44.0000"
    "|sl:helligkeit:0:255:sv=1:c=FF2196F3"
    "@0.0400,0.1143,370.0000,100.0000,0.0400,0.1143,370.0000,100.0000"
    "|di:anzeige:lb=Helligkeit:u=%"
    "@0.0400,0.3143,180.0000,60.0000,0.0400,0.3143,180.0000,60.0000";

// --- Variablen – werden von remote.handle() automatisch aktualisiert ---
int helligkeit = 0;  // Slider-Wert: 0 – 255


void setup() {
  remote.setProfile(PROFILE);  // Profil einbetten – wird beim Verbinden an die App übertragen
  remote.begin();

  remote.receive("helligkeit", helligkeit);

  pinMode(LED_PIN, OUTPUT);
}

void loop() {
  remote.handle();  // Empfangene Befehle verarbeiten und Variablen aktualisieren (erforderlich)

  if (remote.isConnected()) {
    analogWrite(LED_PIN, helligkeit);

    // Prozentwert an Display-Element der App senden
    remote.send("anzeige", map(helligkeit, 0, 255, 0, 100));
  } else {
    analogWrite(LED_PIN, 0);
  }
}
