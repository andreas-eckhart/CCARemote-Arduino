/*
 * CCARemoteBLE.h – Bluetooth Low Energy (BLE) Class
 *
 * Platform detection (automatic):
 *   ESP32            → native BLE (BLEDevice library)
 *   Arduino Uno/Nano → HM-10 module via SoftwareSerial
 *
 * Based on the diploma thesis by L. Eder and E. Duyar (HTL Anichstraße)
 * Extended by A. Eckhart with kind permission of the original authors.
 *
 * MIT – see LICENSE
 */

#ifndef CCAREMOTE_BLE_H
#define CCAREMOTE_BLE_H

#include "CCARemoteBase.h"

// ================================================================
#if defined(ESP32)
// ================================================================
//  ESP32 – Natives BLE
// ================================================================

#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

class CCARemoteBLE : public CCARemote {
  public:
    CCARemoteBLE(String name,
                 String        prefix     = "CCA-",
                 String        password   = "",
                 CCADebugMode  debugLevel = CCA_DEBUG_OFF,
                 unsigned long baudRate   = 115200);

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
    BLECharacteristic* pControlChar;
    BLECharacteristic* pDisplayChar;
    bool               deviceConnected;
    String             blePassword;
    bool               authenticated;

    class ServerCallbacks;
    class ControlCallbacks;
};

// ================================================================
#else
// ================================================================
//  Arduino Uno / Nano – HM-10 BLE-Modul über SoftwareSerial
// ================================================================

#include <SoftwareSerial.h>

class CCARemoteBLE : public CCARemote {
  public:
    // rxPin    – SoftwareSerial RX-Pin am Arduino (Standard: 10) → HM-10 TXD
    // txPin    – SoftwareSerial TX-Pin am Arduino (Standard: 11) → HM-10 RXD
    // Hinweis: Manche Klon-Module haben TXD/RXD vertauscht beschriftet.
    //          Falls keine Verbindung zustande kommt, Leitungen tauschen.
    // baudRate – Baudrate des HM-10-Moduls         (Standard: 9600)
    CCARemoteBLE(String name,
                 String        prefix      = "CCA-",
                 uint8_t       rxPin       = 10,
                 uint8_t       txPin       = 11,
                 uint32_t      hm10Baud    = 9600,
                 String        password    = "",
                 CCADebugMode  debugLevel  = CCA_DEBUG_OFF,
                 unsigned long serialBaud  = 9600);
    ~CCARemoteBLE();

    void begin();
    void handle() override;
    bool isConnected() override;

  protected:
    void sendInternal(String key, String value) override;

  private:
    SoftwareSerial* _serial;
    uint8_t         _rxPin;
    uint8_t         _txPin;
    uint32_t        _baudRate;
    bool            _connected;
    bool            _authenticated;
    String          _blePassword;
    String          _rxBuffer;
    unsigned long   _lastByteTime;

    void _processRx(String data);
    void _sendRaw(String msg);
};

#endif // ESP32

#endif // CCAREMOTE_BLE_H
