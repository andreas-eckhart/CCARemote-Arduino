/*
 * CCARemoteWiFi.h – WiFi Access Point Class
 *
 * Plattform-Erkennung (automatisch):
 *   ESP32   → WiFi.h + WebServer.h
 *   ESP8266 → ESP8266WiFi.h + ESP8266WebServer.h
 *   AVR     → nicht unterstützt (Datei wird leer kompiliert)
 *
 * Based on the diploma thesis by L. Eder and E. Duyar (HTL Anichstraße)
 * Extended by A. Eckhart with kind permission of the original authors.
 *
 * Version: 1.1.0 | 2026-05-07 | MIT – see LICENSE
 */

#ifndef CCAREMOTE_WIFI_H
#define CCAREMOTE_WIFI_H

#if !defined(__AVR__)

#include "CCARemote.h"

#if defined(ESP8266)
  #include <ESP8266WiFi.h>
  #include <ESP8266WebServer.h>
  using CCAWebServer = ESP8266WebServer;
#else
  #include <WiFi.h>
  #include <WebServer.h>
  using CCAWebServer = WebServer;
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
    CCAWebServer* webServer;
    bool          wifiEnabled;

    void handleRoot();
    void handleStatus();
    void handleCommand();
    void handleDisplay();
};

#endif // !__AVR__

#endif // CCAREMOTE_WIFI_H
