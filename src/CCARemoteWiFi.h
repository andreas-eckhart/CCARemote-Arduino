/*
 * CCARemoteWiFi.h – WiFi Access Point Class
 *
 * Based on the diploma thesis by L. Eder and E. Duyar (HTL Anichstraße)
 * Extended by A. Eckhart with kind permission of the original authors.
 *
 * Version: 1.0.0 | 2026-05-03 | MIT – see LICENSE
 */

#ifndef CCAREMOTE_WIFI_H
#define CCAREMOTE_WIFI_H

#include "CCARemote.h"
#include <WiFi.h>
#include <WebServer.h>

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
    WebServer* webServer;
    bool       wifiEnabled;

    void handleRoot();
    void handleStatus();
    void handleCommand();
    void handleDisplay();
};

#endif // CCAREMOTE_WIFI_H