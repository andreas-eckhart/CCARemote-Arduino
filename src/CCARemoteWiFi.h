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
    void handleCommand();
    void handleDisplay();
};

#endif // CCAREMOTE_WIFI_H