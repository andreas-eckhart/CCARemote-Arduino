#ifndef CCAREMOTE_MQTT_H
#define CCAREMOTE_MQTT_H

#include "CCARemote.h"

// CCARemoteMQTT erfordert die Bibliothek "PubSubClient" (Bibliotheks-Manager).
// Falls PubSubClient nicht installiert ist, wird diese Klasse uebersprungen.
#if defined(__has_include) && __has_include(<PubSubClient.h>)

#include <WiFi.h>
#include <WiFiClient.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>

class CCARemoteMQTT : public CCARemote {
  public:
    CCARemoteMQTT(String name, String prefix = "CCA-");
    ~CCARemoteMQTT();

    // MQTT (unverschluesselt, Standard-Port 1883)
    void begin(String wifiSSID, String wifiPassword,
              String brokerHost, uint16_t port = 1883);

    // MQTTS (TLS, Standard-Port 8883)
    // caCert = nullptr -> Zertifikat nicht pruefen (setInsecure)
    void begin(String wifiSSID, String wifiPassword,
              String brokerHost, uint16_t port,
              const char* caCert);

    void handle() override;
    bool isConnected() override;

  protected:
    void sendDisplayInternal(String key, String value) override;

  private:
    String   _brokerHost;
    uint16_t _brokerPort;
    bool     _useTLS;

    WiFiClient*       wifiClient;
    WiFiClientSecure* secureClient;
    PubSubClient*     mqttClient;

    String topicCommand;
    String topicDisplay;

    unsigned long _lastReconnectAttempt;

    void connectWiFi(String ssid, String password);
    bool connectMQTT();

    // Statischer Callback fuer PubSubClient (nur eine Instanz unterstuetzt)
    static CCARemoteMQTT* _instance;
    static void mqttCallback(char* topic, byte* payload, unsigned int length);
};

#else
  #warning "PubSubClient nicht installiert! CCARemoteMQTT nicht verfuegbar."
  #warning "Installiere 'PubSubClient' ueber den Arduino Bibliotheks-Manager."
#endif // __has_include(<PubSubClient.h>)

#endif // CCAREMOTE_MQTT_H