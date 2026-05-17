/*
 * CCARemoteBLE.cpp – Bluetooth Low Energy (BLE) Implementation
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

#include "CCARemoteBLE.h"

// ================================================================
#if defined(ESP32)
// ================================================================
//  ESP32 – Natives BLE
// ================================================================

const char* CCARemoteBLE::SERVICE_UUID = "4fafc201-1fb5-459e-8fcc-c5c9c331914b";
const char* CCARemoteBLE::CONTROL_UUID = "cba1d466-344c-4be3-ab3f-189f80dd7518"; // App → Arduino
const char* CCARemoteBLE::DISPLAY_UUID = "d9a98a3e-7c1f-4b2a-9e8f-6d2c3a1b5e7f"; // Arduino → App

class CCARemoteBLE::ServerCallbacks : public BLEServerCallbacks {
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

class CCARemoteBLE::ControlCallbacks : public BLECharacteristicCallbacks {
  CCARemoteBLE* parent;
public:
  ControlCallbacks(CCARemoteBLE* p) : parent(p) {}
  void onWrite(BLECharacteristic* pChar) override {
    String value = pChar->getValue().c_str();
    value.trim();
    if (value.length() == 0) return;

    if (!parent->blePassword.length() == 0 && !parent->authenticated) {
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

    parent->lastCommand     = value;
    parent->commandReceived = true;
  }
};

CCARemoteBLE::CCARemoteBLE(String name, String prefix,
                           String password, CCADebugMode debugLevel,
                           unsigned long baudRate)
  : CCARemote(name, prefix, debugLevel, baudRate)
{
  blePassword     = password;
  pServer         = nullptr;
  pControlChar    = nullptr;
  pDisplayChar    = nullptr;
  deviceConnected = false;
  authenticated   = false;
}

void CCARemoteBLE::begin() {
  Serial.begin(_serialBaudRate);
  Serial.println("\nCCA Remote startet (BLE)...");
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
  pDisplayChar->addDescriptor(new BLE2902());

  pService->start();

  BLEAdvertising* pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x06);
  pAdvertising->setMaxPreferred(0x12);
  BLEDevice::startAdvertising();

  Serial.println("BLE Server laeuft!");
  if (blePassword.length() > 0) {
    Serial.println("Passwort aktiv: AUTH-Befehl erforderlich.");
  }
  Serial.println("Warte auf Verbindung...\n");
}

void CCARemoteBLE::handle() {
  _checkWatchdogs();
  if (_pendingResync && deviceConnected && authenticated) {
    _pendingResync = false;
    _resyncDisplay();
  }
  if (commandReceived) {
    processCommand(lastCommand);
    commandReceived = false;
  }
}

bool CCARemoteBLE::isConnected() {
  return deviceConnected && authenticated;
}

void CCARemoteBLE::sendInternal(String key, String value) {
  if (deviceConnected && authenticated && pDisplayChar != nullptr) {
    String msg = key + ":" + value + "\n";
    pDisplayChar->setValue(msg.c_str());
    pDisplayChar->notify();
  }
}

// ================================================================
#else
// ================================================================
//  Arduino Uno / Nano – HM-10 BLE-Modul über SoftwareSerial
// ================================================================

CCARemoteBLE::CCARemoteBLE(String name, String prefix,
                           uint8_t rxPin, uint8_t txPin,
                           uint32_t hm10Baud,
                           String password, CCADebugMode debugLevel,
                           unsigned long serialBaud)
  : CCARemote(name, prefix, debugLevel, serialBaud),
    _serial(nullptr),
    _rxPin(rxPin), _txPin(txPin), _baudRate(hm10Baud),
    _connected(false), _authenticated(false),
    _blePassword(password),
    _rxBufLen(0),
    _lastByteTime(0)
{
  _rxBuf[0] = '\0';
}

CCARemoteBLE::~CCARemoteBLE() {
  delete _serial;
}

void CCARemoteBLE::begin() {
  Serial.begin(_serialBaudRate);
  Serial.println(F("\nCCA Remote startet (BLE/HM-10)..."));
  Serial.print(F("Geraetename: "));
  Serial.println(deviceName);

  _serial = new SoftwareSerial(_rxPin, _txPin);
  _serial->begin(_baudRate);
  delay(500);

  // Eingangspuffer leeren
  while (_serial->available()) _serial->read();

  // Geraetename setzen (HM-10: max. 12 Zeichen)
  String n = deviceName.substring(0, min((int)deviceName.length(), 12));
  _serial->print("AT+NAME" + n);
  delay(200);

  // Slave-Modus sicherstellen
  _serial->print("AT+ROLE0");
  delay(200);

  // Neustart damit Einstellungen uebernommen werden
  _serial->print("AT+RESET");
  delay(800);

  // Puffer nach Reset leeren
  while (_serial->available()) _serial->read();

  Serial.print(F("HM-10 bereit! (RX="));
  Serial.print(_rxPin);
  Serial.print(F(", TX="));
  Serial.print(_txPin);
  Serial.println(')');
  if (_blePassword.length() > 0) {
    Serial.println(F("Passwort aktiv: AUTH-Befehl erforderlich."));
  }
  Serial.println(F("Warte auf Verbindung...\n"));
}

void CCARemoteBLE::handle() {
  _checkWatchdogs();
  if (_pendingResync && _connected && _authenticated) {
    _pendingResync = false;
    _resyncDisplay();
  }

  // Bytes lesen – bei \n sofort verarbeiten, sonst in statischem Puffer sammeln
  while (_serial->available()) {
    char c = (char)_serial->read();

    if (c == '\n') {
      if (_rxBufLen > 0) {
        _lastByteTime = 0;
        _dispatchRx();
      }
    } else if ((uint8_t)c >= 0x20 && (uint8_t)c < 0x7F) {
      if (_rxBufLen < sizeof(_rxBuf) - 1) {
        _rxBuf[_rxBufLen++] = c;
      }
      _lastByteTime = millis();
    }
  }

  // Fallback: nach 100 ms ohne \n → Puffer trotzdem verarbeiten (AT-Antworten des HM-10)
  if (_rxBufLen > 0 && _lastByteTime > 0 &&
      (millis() - _lastByteTime) >= 100) {
    _lastByteTime = 0;
    _dispatchRx();
  }

}

bool CCARemoteBLE::isConnected() {
  return _connected && _authenticated;
}

void CCARemoteBLE::sendInternal(String key, String value) {
  if (_connected && _authenticated) {
    _sendRaw(key + ":" + value + "\n");
  }
}

void CCARemoteBLE::_dispatchRx() {
  _rxBuf[_rxBufLen] = '\0';
  _rxBufLen = 0;

  // In-place trimmen: führende/abschließende Leerzeichen entfernen
  char* p = _rxBuf;
  while (*p == ' ') p++;
  uint8_t len = strlen(p);
  while (len > 0 && p[len - 1] == ' ') p[--len] = '\0';

  if (len > 0) _processRx(p);
}

void CCARemoteBLE::_processRx(const char* raw) {
  // if (debugMode != CCA_DEBUG_OFF) {
  //   Serial.print(F("[CCA] HM-10 RX: \""));
  //   Serial.print(raw);
  //   Serial.println('"');
  // }

  // Verbindungs-Events (je nach HM-10-Firmware-Version)
  // strstr() arbeitet direkt auf dem char* – kein String-Heap nötig
  if (strstr(raw, "OK+CONN") || strstr(raw, "+CONNECTED") ||
      strcmp(raw, "CONNECTED") == 0) {
    _connected     = true;
    _authenticated = _blePassword.length() == 0;
    _pendingResync = true;
    if (debugMode != CCA_DEBUG_OFF) Serial.println(F("[CCA] Verbindung hergestellt"));
    return;
  }
  if (strstr(raw, "OK+LOST") || strstr(raw, "+DISCONNECTED") || strstr(raw, "DISCONNECT")) {
    _connected     = false;
    _authenticated = false;
    _lastByteTime  = 0;
    Serial.println(F("[CCA] Verbindung getrennt"));
    return;
  }

  // AT-Antworten des Moduls ignorieren
  if (strncmp(raw, "OK+", 3) == 0) return;

  // Viele HM-10 Klone senden kein Verbindungs-Event → beim ersten Datenpaket implizit verbinden
  if (!_connected) {
    _connected     = true;
    _authenticated = _blePassword.length() == 0;
    _pendingResync = true;
    if (debugMode != CCA_DEBUG_OFF) Serial.println(F("[CCA] Verbindung hergestellt (implizit)"));
  }

  // Authentifizierung prüfen, falls Passwort gesetzt
  // strstr statt strcmp: toleriert Garbage-Bytes vor AUTH: (HM-10 AT-Antworten)
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
    // kein AUTH: gefunden → Garbage vor dem ersten Write, ignorieren
    return;
  }

  if (!_authenticated) return;
  if (strcmp(raw, "AUTH") == 0 || strncmp(raw, "AUTH:", 5) == 0) {
    // AUTH-Befehl der App = Subscription aktiv → Display-Werte jetzt sicher senden
    _pendingResync = true;
    return;
  }

  // Garbage-Filter: HM-10 injiziert AT-Response-Bytes vor den Nutzdaten.
  // CCA-Keys beginnen immer mit Kleinbuchstabe (color1, axisX, button1, ...).
  // Ein echter Key enthält keinen Großbuchstaben direkt vor einem Kleinbuchstaben
  // (axisX ist ok: X steht am Ende; swRcolor1 ist kein Key: R→c ist ungültig).
  {
    const char* cmdStart = nullptr;
    for (const char* p = raw; *p; p++) {
      if (islower((uint8_t)*p)) {
        const char* q = p;
        while (isalnum((uint8_t)*q)) q++;
        if (*q == ':' || *q == '\0') {
          bool valid = true;
          for (const char* r = p; r < q - 1; r++) {
            if (isupper((uint8_t)*r) && islower((uint8_t)*(r + 1))) {
              valid = false;
              break;
            }
          }
          if (valid) { cmdStart = p; break; }
        }
      }
    }
    if (!cmdStart) return;  // kein gültiger Key gefunden → Garbage verwerfen
    raw = cmdStart;
  }

  // Einmalige String-Allokation erst hier, wo processCommand() sie braucht
  processCommand(String(raw));
}

void CCARemoteBLE::_sendRaw(String msg) {
  if (_serial) _serial->print(msg);
}

#endif // ESP32
