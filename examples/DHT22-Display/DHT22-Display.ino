// ============================================================
//  CCA Remote Beispiel: DHT22-Sensor mit Display-Element
//  Ein am Controller angeschlossener DHT22 Sensor sendet
//  regelmäßig Temperatur und Luftfeuchtigkeit an die App.
//  Erfordert "DHT sensor library" von Adafruit.
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

const int DHT_PIN = 4;

#include <DHT.h>
#define DHTTYPE DHT22
DHT dht(DHT_PIN, DHTTYPE);

unsigned long lastUpdate = 0;
const unsigned long UPDATE_INTERVAL = 5000;


void setup() {
  remote.begin();
  dht.begin();
}

void loop() {
  remote.handle();  // Empfangene Befehle verarbeiten und Variablen aktualisieren (erforderlich)

  // Sensorwerte senden (nur wenn App verbunden)
  if (remote.isConnected() && millis() - lastUpdate >= UPDATE_INTERVAL) {
    lastUpdate = millis();

    float temperature = dht.readTemperature();
    float humidity    = dht.readHumidity();

    if (!isnan(temperature) && !isnan(humidity)) {
      remote.send("temp1",  temperature);  // z.B. "23.5"
      remote.send("humid1", humidity);     // z.B. "58.0"
    }
  }
}
