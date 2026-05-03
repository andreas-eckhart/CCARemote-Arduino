/*
 * CCARemoteMQTT.cpp – MQTT Implementation
 *
 * Based on the diploma thesis by L. Eder and E. Duyar (HTL Anichstraße)
 * Extended by A. Eckhart with kind permission of the original authors.
 *
 * Version: 1.0.0 | 2026-05-03 | MIT – see LICENSE
 */

#include "CCARemoteMQTT.h"

#if defined(__has_include) && __has_include(<PubSubClient.h>)

CCARemoteMQTT* CCARemoteMQTT::_instance = nullptr;

CCARemoteMQTT::CCARemoteMQTT(String name, String prefix) : CCARemote(name, prefix) {
  _brokerPort           = 0;
  _useTLS               = false;
  wifiClient            = nullptr;
  secureClient          = nullptr;
  mqttClient            = nullptr;
  _lastReconnectAttempt = 0;
  _instance             = this;
}

CCARemoteMQTT::~CCARemoteMQTT() {
  delete mqttClient;
  delete wifiClient;
  delete secureClient;
}

void CCARemoteMQTT::begin(String wifiSSID, String wifiPassword,
                           String brokerHost, uint16_t port) {
  _brokerHost = brokerHost;
  _brokerPort = port;
  _useTLS     = false;

  Serial.begin(115200);
  Serial.println("\nCCA Remote startet (MQTT)...");
  Serial.println("Geraetename: " + deviceName);

  connectWiFi(wifiSSID, wifiPassword);

  wifiClient = new WiFiClient();
  mqttClient = new PubSubClient(*wifiClient);
  mqttClient->setServer(brokerHost.c_str(), port);
  mqttClient->setCallback(mqttCallback);

  topicCommand = "cca/" + deviceName + "/cmd";
  topicDisplay = "cca/" + deviceName + "/display";

  Serial.println("MQTT Broker: " + brokerHost + ":" + String(port));
  Serial.println("Befehl-Topic:  " + topicCommand);
  Serial.println("Display-Topic: " + topicDisplay);

  while (!connectMQTT()) {
    Serial.println("Erneuter Versuch in 2s...");
    delay(2000);
  }
  Serial.println("CCA Remote bereit!\n");
}

void CCARemoteMQTT::begin(String wifiSSID, String wifiPassword,
                           String brokerHost, uint16_t port,
                           const char* caCert) {
  _brokerHost = brokerHost;
  _brokerPort = port;
  _useTLS     = true;

  Serial.begin(115200);
  Serial.println("\nCCA Remote startet (MQTTS)...");
  Serial.println("Geraetename: " + deviceName);

  connectWiFi(wifiSSID, wifiPassword);

  secureClient = new WiFiClientSecure();
  if (caCert != nullptr) {
    secureClient->setCACert(caCert);
    Serial.println("TLS: CA-Zertifikat gesetzt");
  } else {
    secureClient->setInsecure();
    Serial.println("TLS: Zertifikat wird nicht geprueft (insecure)");
  }

  mqttClient = new PubSubClient(*secureClient);
  mqttClient->setServer(brokerHost.c_str(), port);
  mqttClient->setCallback(mqttCallback);

  topicCommand = "cca/" + deviceName + "/cmd";
  topicDisplay = "cca/" + deviceName + "/display";

  Serial.println("MQTTS Broker: " + brokerHost + ":" + String(port));
  Serial.println("Befehl-Topic:  " + topicCommand);
  Serial.println("Display-Topic: " + topicDisplay);

  while (!connectMQTT()) {
    Serial.println("Erneuter Versuch in 2s...");
    delay(2000);
  }
  Serial.println("CCA Remote bereit!\n");
}

void CCARemoteMQTT::connectWiFi(String ssid, String password) {
  Serial.print("Verbinde mit WiFi: " + ssid);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid.c_str(), password.c_str());
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.print("WiFi verbunden, IP: ");
  Serial.println(WiFi.localIP());
}

bool CCARemoteMQTT::connectMQTT() {
  if (mqttClient == nullptr) return false;
  Serial.print("Verbinde mit MQTT Broker...");
  if (mqttClient->connect(deviceName.c_str())) {
    Serial.println(" verbunden!");
    mqttClient->subscribe(topicCommand.c_str());
    return true;
  }
  Serial.print(" Fehler (");
  Serial.print(mqttClient->state());
  Serial.println(")");
  return false;
}

void CCARemoteMQTT::mqttCallback(char* topic, byte* payload, unsigned int length) {
  if (_instance == nullptr) return;
  String message;
  for (unsigned int i = 0; i < length; i++) {
    message += (char)payload[i];
  }
  _instance->lastCommand     = message;
  _instance->commandReceived = true;
}

void CCARemoteMQTT::handle() {
  if (mqttClient == nullptr) return;

  if (!mqttClient->connected()) {
    unsigned long now = millis();
    if (now - _lastReconnectAttempt >= 5000) {
      _lastReconnectAttempt = now;
      connectMQTT();
    }
    return;
  }

  mqttClient->loop();

  if (commandReceived) {
    processCommand(lastCommand);
    commandReceived = false;
  }
}

bool CCARemoteMQTT::isConnected() {
  return mqttClient != nullptr && mqttClient->connected();
}

void CCARemoteMQTT::sendDisplayInternal(String key, String value) {
  if (mqttClient != nullptr && mqttClient->connected()) {
    bool changed = (displayValues.find(key) == displayValues.end()) ||
                   (displayValues[key] != value);
    displayValues[key] = value;
    String msg = key + ":" + value;
    mqttClient->publish(topicDisplay.c_str(), msg.c_str());
    if (changed) {
      Serial.println("Display: " + key + " = " + value);
    }
  }
}

#endif // __has_include(<PubSubClient.h>)