// ============================================================
//  CCA Remote Beispiel: Display Element
//  Ein am Controller angeschlossener DHT22 Sensor sendet
//  regelmäßig Temperatur und Luftfeuchtigkeit an die App.
//  erfordert "DHT sensor library" von Adafruit
// ============================================================
//  Erfordert die kostenlose CCA Remote App (Android / iOS)
// ============================================================

// Für BLE Verbindung
#include <CCARemoteBLE.h>
CCARemoteBLE remote("MeinName");  // Namen hier anpassen!

// Für WiFi Verbindung
// #include <CCARemoteWiFi.h>
// CCARemoteWiFi remote("MeinName");  // Namen hier anpassen!

const int DHT_PIN = 4;

#include <DHT.h>  // DHT Bibliothek laden
#define DHTTYPE DHT22
DHT dht(DHT_PIN, DHTTYPE);

unsigned long lastUpdate = 0;
const unsigned long UPDATE_INTERVAL = 5000;


void setup() {
  remote.debug(CCA_DEBUG_ALL, 9600);  // Debug Modus: CCA_DEBUG_IN, CCA_DEBUG_OUT oder CCA_DEBUG_ALL
  remote.begin();            // Initialisierung (erforderlich)

  dht.begin();    // DHT-Sensor starten

}

void loop() {
  remote.handle();  // Empfangene Befehle verarbeiten und Variablen aktualisieren (erforderlich)

  // Sensorwerte senden (nur wenn App verbunden)
  if (remote.isConnected() && millis() - lastUpdate >= UPDATE_INTERVAL) {
    lastUpdate = millis();
    
    float temperature = dht.readTemperature();
    float humidity = dht.readHumidity();
    
    if (!isnan(temperature) && !isnan(humidity)) {
      remote.send("temp1", temperature);  // z.B. "23.5"
      remote.send("humid1", humidity);    // z.B. "58.0"
    }
  }
}
