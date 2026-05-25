/*
 * CCARemoteBLE.h – Bluetooth Low Energy (BLE) Class (header-only)
 *
 * Platform detection (automatic):
 *   ESP32            → native BLE (BLEDevice library)
 *   Arduino Uno/Nano → HM-10 module via SoftwareSerial
 *
 * Header-only so that CCARemoteBLE.cpp is never compiled in WiFi sketches,
 * preventing the BLE library from being linked into WiFi builds.
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
#ifndef CONFIG_NIMBLE_ENABLED
  #include <BLE2902.h>
#endif

class CCARemoteBLE : public CCARemote {
  public:
    CCARemoteBLE(String name,
                 String        prefix     = "CCA-",
                 String        password   = "",
                 CCADebugMode  debugLevel = CCA_DEBUG_OFF,
                 unsigned long baudRate   = 115200)
      : CCARemote(name, prefix, debugLevel, baudRate),
        blePassword(password), pServer(nullptr),
        pControlChar(nullptr), pDisplayChar(nullptr),
        deviceConnected(false), authenticated(false)
    {}

    void begin() {
      Serial.begin(_serialBaudRate);
      Serial.println("\nCCA Remote startet (BLE)...");
      if (debugMode == CCA_DEBUG_ALL)
        Serial.println("[CCA] Library: " CCA_LIB_VERSION "  |  Protokoll: " CCA_PROTOCOL_VERSION);
      Serial.println("Geraetename: " + deviceName);

      BLEDevice::init(deviceName.c_str());
      pServer = BLEDevice::createServer();
      pServer->setCallbacks(new ServerCallbacks(this));

      BLEService* pService = pServer->createService(SERVICE_UUID);

      pControlChar = pService->createCharacteristic(
        CONTROL_UUID,
        BLECharacteristic::PROPERTY_WRITE |
        BLECharacteristic::PROPERTY_WRITE_NR
      );
      pControlChar->setCallbacks(new ControlCallbacks(this));

      pDisplayChar = pService->createCharacteristic(
        DISPLAY_UUID,
        BLECharacteristic::PROPERTY_READ  |
        BLECharacteristic::PROPERTY_NOTIFY
      );
      #ifndef CONFIG_NIMBLE_ENABLED
      pDisplayChar->addDescriptor(new BLE2902());
      #endif

      pService->start();

      BLEAdvertising* pAdvertising = BLEDevice::getAdvertising();
      pAdvertising->addServiceUUID(SERVICE_UUID);
      pAdvertising->setScanResponse(true);
      pAdvertising->setMinPreferred(0x06);
      pAdvertising->setMaxPreferred(0x12);
      BLEDevice::startAdvertising();

      Serial.println("BLE Server laeuft!");
      if (blePassword.length() > 0) {
        Serial.print("BLE Passwort: ");
        if (debugMode == CCA_DEBUG_ALL) {
          Serial.println(blePassword);
        } else {
          for (unsigned int i = 0; i < blePassword.length(); i++) Serial.print('*');
          Serial.println();
        }
      }
      Serial.println("Warte auf Verbindung...\n");
    }

    void handle() override {
      _checkWatchdogs();
      if (_pendingResync && deviceConnected && authenticated) {
        _pendingResync = false;
        _resyncDisplay();
      }
      if (commandReceived) {
        processCommand(lastCommand.c_str());
        commandReceived = false;
      }
    }

    bool isConnected() override {
      return deviceConnected && authenticated;
    }

  protected:
    void sendInternal(String key, String value) override {
      if (!deviceConnected || !authenticated || pDisplayChar == nullptr) return;
      String msg = key + ":" + value + "\n";
      // BLE notifications are capped at MTU-3 bytes per packet.
      // For long messages (e.g. profileConfig) we chunk and send multiple
      // notifications; the Flutter side reassembles them via _rxBuffer.
      const size_t kChunk = 180;
      if (msg.length() <= kChunk) {
        pDisplayChar->setValue(msg.c_str());
        pDisplayChar->notify();
      } else {
        size_t offset = 0;
        while (offset < msg.length()) {
          size_t end = min(offset + kChunk, msg.length());
          String chunk = msg.substring(offset, end);
          pDisplayChar->setValue(chunk.c_str());
          pDisplayChar->notify();
          offset = end;
          if (offset < msg.length()) delay(10);
        }
      }
    }

  private:
    static constexpr const char* SERVICE_UUID = "4fafc201-1fb5-459e-8fcc-c5c9c331914b";
    static constexpr const char* CONTROL_UUID = "cba1d466-344c-4be3-ab3f-189f80dd7518";
    static constexpr const char* DISPLAY_UUID = "d9a98a3e-7c1f-4b2a-9e8f-6d2c3a1b5e7f";

    BLEServer*         pServer;
    BLECharacteristic* pControlChar;
    BLECharacteristic* pDisplayChar;
    bool               deviceConnected;
    String             blePassword;
    bool               authenticated;

    class ServerCallbacks : public BLEServerCallbacks {
      CCARemoteBLE* parent;
    public:
      ServerCallbacks(CCARemoteBLE* p) : parent(p) {}
      void onConnect(BLEServer*) override {
        parent->deviceConnected = true;
        parent->authenticated   = parent->blePassword.length() == 0;
        parent->_pendingResync  = true;
        if (parent->debugMode != CCA_DEBUG_OFF) Serial.println("[CCA] Verbindung hergestellt");
      }
      void onDisconnect(BLEServer*) override {
        parent->deviceConnected = false;
        parent->authenticated   = false;
        if (parent->debugMode != CCA_DEBUG_OFF) Serial.println("[CCA] Verbindung getrennt");
        BLEDevice::startAdvertising();
      }
    };

    class ControlCallbacks : public BLECharacteristicCallbacks {
      CCARemoteBLE* parent;
    public:
      ControlCallbacks(CCARemoteBLE* p) : parent(p) {}
      void onWrite(BLECharacteristic* pChar) override {
        String value = pChar->getValue().c_str();
        value.trim();
        if (value.length() == 0) return;
        int gteIdx = value.indexOf('>');
        if (gteIdx >= 0) { value = value.substring(gteIdx + 1); if (value.length() == 0) return; }

        if (parent->blePassword.length() > 0 && !parent->authenticated) {
          if (value == "AUTH:" + parent->blePassword) {
            parent->authenticated  = true;
            parent->_pendingResync = true;
            Serial.println("BLE Authentifizierung erfolgreich!");
            parent->pDisplayChar->setValue("AUTH:OK\n");
            parent->pDisplayChar->notify();
          } else {
            Serial.println("BLE Authentifizierung fehlgeschlagen! Verbindung wird getrennt.");
            parent->pDisplayChar->setValue("AUTH:FAIL\n");
            parent->pDisplayChar->notify();
            delay(50);
            parent->pServer->disconnect(parent->pServer->getConnId());
          }
          return;
        }

        if (value == "AUTH" || value.startsWith("AUTH:")) {
          parent->_pendingResync = true;
          return;
        }

        if (value.startsWith("ping:")) return;
        if (value.startsWith("disconnect:")) return;

        parent->lastCommand     = value;
        parent->commandReceived = true;
      }
    };
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
                 unsigned long serialBaud  = 9600)
      : CCARemote(name, prefix, debugLevel, serialBaud),
        _serial(nullptr),
        _rxPin(rxPin), _txPin(txPin), _baudRate(hm10Baud),
        _connected(false), _authenticated(false),
        _blePassword(password),
        _rxBufLen(0), _lastByteTime(0), _lastRxMs(0), _disconnectLockout(0)
    {
      _rxBuf[0] = '\0';
    }

    ~CCARemoteBLE() {
      delete _serial;
    }

    void begin() {
      Serial.begin(_serialBaudRate);
      Serial.println(F("\nCCA Remote startet (BLE/HM-10)..."));
      Serial.print(F("Geraetename: "));
      Serial.println(deviceName);
      if (debugMode == CCA_DEBUG_ALL)
        Serial.println(F("[CCA] Lib: " CCA_LIB_VERSION "  |  Protokoll: " CCA_PROTOCOL_VERSION));

      _serial = new SoftwareSerial(_rxPin, _txPin);
      _serial->begin(_baudRate);
      delay(500);

      while (_serial->available()) _serial->read();

      String n = deviceName.substring(0, min((int)deviceName.length(), 12));
      _serial->print("AT+NAME" + n);
      delay(200);

      _serial->print("AT+NOTI1");
      delay(200);

      _serial->print("AT+ROLE0");
      delay(200);

      _serial->print("AT+RESET");
      delay(800);

      while (_serial->available()) _serial->read();

      Serial.print(F("HM-10 BLE-Modul bereit! (RX="));
      Serial.print(_rxPin);
      Serial.print(F(", TX="));
      Serial.print(_txPin);
      Serial.println(')');
      if (_blePassword.length() > 0) {
        Serial.print(F("BLE Passwort: "));
        if (debugMode == CCA_DEBUG_ALL) {
          Serial.println(_blePassword);
        } else {
          for (unsigned int i = 0; i < _blePassword.length(); i++) Serial.print('*');
          Serial.println();
        }
      }
      Serial.println(F("Warte auf Verbindung...\n"));
    }

    void handle() override {
      _checkWatchdogs();
      if (_pendingResync && _connected && _authenticated) {
        _pendingResync = false;
        _resyncDisplay();
      }

      while (_serial->available()) {
        char c = (char)_serial->read();
        if (c == '\n') {
          if (_rxBufLen > 0) { _lastByteTime = 0; _dispatchRx(); }
        } else if ((uint8_t)c >= 0x20 && (uint8_t)c < 0x7F) {
          if (_rxBufLen < sizeof(_rxBuf) - 1) _rxBuf[_rxBufLen++] = c;
          _lastByteTime = millis();
        }
      }

      if (_rxBufLen > 0 && _lastByteTime > 0 && (millis() - _lastByteTime) >= 100) {
        _lastByteTime = 0;
        _dispatchRx();
      }

      if (_connected && _lastRxMs > 0 && (millis() - _lastRxMs) >= 6000UL) {
        _connected         = false;
        _authenticated     = false;
        _lastRxMs          = 0;
        _disconnectLockout = millis();
        if (debugMode != CCA_DEBUG_OFF) Serial.println(F("[CCA] Verbindung getrennt (Timeout)"));
      }
    }

    bool isConnected() override {
      return _connected && _authenticated;
    }

  protected:
    void sendInternal(String key, String value) override {
      if (_connected && _authenticated) _sendRaw(key + ":" + value + "\n");
    }

  private:
    SoftwareSerial* _serial;
    uint8_t         _rxPin;
    uint8_t         _txPin;
    uint32_t        _baudRate;
    bool            _connected;
    bool            _authenticated;
    String          _blePassword;
    char            _rxBuf[128];
    uint8_t         _rxBufLen;
    unsigned long   _lastByteTime;
    unsigned long   _lastRxMs;
    unsigned long   _disconnectLockout;

    void _dispatchRx() {
      _rxBuf[_rxBufLen] = '\0';
      _rxBufLen = 0;
      char* p = _rxBuf;
      while (*p == ' ') p++;
      uint8_t len = strlen(p);
      while (len > 0 && p[len - 1] == ' ') p[--len] = '\0';
      if (len > 0) _processRx(p);
    }

    void _processRx(const char* raw) {
      const char* gte = strchr(raw, '>');
      if (gte != nullptr) { raw = gte + 1; if (*raw == '\0') return; }

      if (strstr(raw, "OK+CONN") || strstr(raw, "+CONNECTED") || strcmp(raw, "CONNECTED") == 0) {
        _connected          = true;
        _authenticated      = _blePassword.length() == 0;
        _pendingResync      = true;
        _lastRxMs           = 0;
        _disconnectLockout  = 0;
        if (debugMode != CCA_DEBUG_OFF) Serial.println(F("[CCA] Verbindung hergestellt"));
        return;
      }
      if (strstr(raw, "OK+LOST") || strstr(raw, "+DISCONNECTED") || strstr(raw, "DISCONNECT")) {
        _connected          = false;
        _authenticated      = false;
        _lastByteTime       = 0;
        _lastRxMs           = 0;
        _disconnectLockout  = millis();
        Serial.println(F("[CCA] Verbindung getrennt"));
        return;
      }

      if (strncmp(raw, "OK+", 3) == 0) return;

      if (!_connected) {
        if (_disconnectLockout > 0 && (millis() - _disconnectLockout) < 6000UL) return;
        _connected          = true;
        _authenticated      = _blePassword.length() == 0;
        _pendingResync      = true;
        _disconnectLockout  = 0;
        if (debugMode != CCA_DEBUG_OFF) Serial.println(F("[CCA] Verbindung hergestellt (implizit)"));
      }

      if (_blePassword.length() > 0 && !_authenticated) {
        String authExpected = "AUTH:" + _blePassword;
        if (strstr(raw, authExpected.c_str()) != nullptr) {
          _authenticated = true;
          _pendingResync = true;
          Serial.println(F("BLE Authentifizierung erfolgreich!"));
          _sendRaw("AUTH:OK\n");
        } else if (strstr(raw, "AUTH:") != nullptr) {
          Serial.println(F("BLE Authentifizierung fehlgeschlagen!"));
          _sendRaw("AUTH:FAIL\n");
          _connected = false;
        }
        return;
      }

      if (!_authenticated) return;
      if (strcmp(raw, "AUTH") == 0 || strncmp(raw, "AUTH:", 5) == 0) {
        _pendingResync = true;
        return;
      }

      {
        const char* cmdStart = nullptr;
        for (const char* p = raw; *p; p++) {
          if (islower((uint8_t)*p)) {
            const char* q = p;
            while (isalnum((uint8_t)*q)) q++;
            if (*q == ':' || *q == '\0') {
              bool valid = true;
              for (const char* r = p; r < q - 1; r++) {
                if (isupper((uint8_t)*r) && islower((uint8_t)*(r + 1))) { valid = false; break; }
              }
              if (valid) { cmdStart = p; break; }
            }
          }
        }
        if (!cmdStart) return;
        raw = cmdStart;
      }

      _lastRxMs = millis();

      if (strncmp(raw, "ping:", 5) == 0) return;

      if (strncmp(raw, "disconnect:", 11) == 0) {
        _connected         = false;
        _authenticated     = false;
        _lastRxMs          = 0;
        _disconnectLockout = millis();
        if (debugMode != CCA_DEBUG_OFF) Serial.println(F("[CCA] Verbindung getrennt (App)"));
        return;
      }

      processCommand(raw);
    }

    void _sendRaw(String msg) {
      if (_serial) _serial->print(msg);
    }

#if defined(__AVR__)
    // Streams profileConfig byte-by-byte – zero heap allocation.
    // PROGMEM strings are read via Print::print(const __FlashStringHelper*).
    void _sendProfileConfig() override {
      if (!_connected || !_authenticated || !_serial) return;
      _serial->print(F("profileConfig:"));
      if (_profileIsPgm)
        _serial->print((const __FlashStringHelper*)_profileConfigPtr);
      else
        _serial->print(_profileConfigPtr);
      _serial->write('\n');
    }
#endif
};

#endif // ESP32

#endif // CCAREMOTE_BLE_H
