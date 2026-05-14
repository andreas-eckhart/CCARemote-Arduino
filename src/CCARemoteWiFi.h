/*
 * CCARemoteWiFi.h – WiFi Access Point Class
 *
 * Platform detection (automatic):
 *   ESP32   → WiFi.h
 *   ESP8266 → ESP8266WiFi.h
 *   AVR     → not supported (file compiles empty)
 *
 * Based on the diploma thesis by L. Eder and E. Duyar (HTL Anichstraße)
 * Extended by A. Eckhart with kind permission of the original authors.
 *
 * MIT – see LICENSE
 */

#ifndef CCAREMOTE_WIFI_H
#define CCAREMOTE_WIFI_H

#if !defined(__AVR__)

#include "CCARemote.h"

#if defined(ESP8266)
  #include <ESP8266WiFi.h>
#else
  #include <WiFi.h>
#endif

class CCARemoteWiFi : public CCARemote {
  public:
    CCARemoteWiFi(String name, String prefix = "CCA-");
    ~CCARemoteWiFi();

    // wifiPassword = "" -> offenes Netzwerk
    void begin(String wifiPassword = "");
    void handle() override;
    bool isConnected() override;

  protected:
    void sendInternal(String key, String value) override;

  private:
    bool        wifiEnabled;
    bool        _wasConnected;

    WiFiServer* _tcpServer;
    WiFiClient  _tcpClient;
    String      _tcpBuf;

    void _tcpDisconnect();
};

#endif // !__AVR__

#endif // CCAREMOTE_WIFI_H
