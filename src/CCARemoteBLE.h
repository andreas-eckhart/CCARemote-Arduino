/*
 * CCARemoteBLE.h – Bluetooth Low Energy (BLE) Class
 *
 * Based on the diploma thesis by L. Eder and E. Duyar (HTL Anichstraße)
 * Extended by A. Eckhart with kind permission of the original authors.
 *
 * Version: 1.0.0 | 2026-05-03 | MIT – see LICENSE
 */

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
    // blePassword = "" -> keine Authentifizierung erforderlich
    void begin(String blePassword = "");
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
    String             blePassword;
    bool               authenticated;

    class ServerCallbacks;
    class ControlCallbacks;
};

#endif // CCAREMOTE_BLE_H