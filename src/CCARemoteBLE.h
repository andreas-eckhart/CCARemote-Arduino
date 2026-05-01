#ifndef CCAREMOTE_BLE_H
#define CCAREMOTE_BLE_H

#include "CCARemote.h"
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

class CCARemoteBLE : public CCARemote {
  public:
    CCARemoteBLE(String name, String prefix = "CCA-");
    void begin();
    void handle() override;
    bool isConnected() override;

  protected:
    void sendInternal(String key, String value) override;

  private:
    static const char* SERVICE_UUID;
    static const char* CONTROL_UUID;  // App → Arduino (WRITE)
    static const char* DISPLAY_UUID;  // Arduino → App (NOTIFY)

    BLEServer*         pServer;
    BLECharacteristic* pControlChar;   // empfängt Befehle
    BLECharacteristic* pDisplayChar;   // sendet Display-Werte
    bool               deviceConnected;

    class ServerCallbacks;
    class ControlCallbacks;
};

#endif // CCAREMOTE_BLE_H