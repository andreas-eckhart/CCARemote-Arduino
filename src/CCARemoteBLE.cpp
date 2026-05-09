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
    if (value.length() == 0) return;

    if (!parent->blePassword.length() == 0 && !parent->authenticated) {
      if (value == "AUTH:" + parent->blePassword) {
        parent->authenticated  = true;
        parent->_pendingResync = true;
        Serial.println("BLE Authentifizierung erfolgreich!");
        parent->pDisplayChar->setValue("AUTH:OK");
        parent->pDisplayChar->notify();
      } else {
        Serial.println("BLE Authentifizierung fehlgeschlagen! Verbindung wird getrennt.");
        parent->pDisplayChar->setValue("AUTH:FAIL");
        parent->pDisplayChar->notify();
        delay(50);
        parent->pServer->disconnect(parent->pServer->getConnId());
      }
      return;
    }

    parent->lastCommand     = value;
    parent->commandReceived = true;
  }
};

CCARemoteBLE::CCARemoteBLE(String name, String prefix) : CCARemote(name, prefix) {
  pServer         = nullptr;
  pControlChar    = nullptr;
  pDisplayChar    = nullptr;
  deviceConnected = false;
  authenticated   = false;
}

void CCARemoteBLE::begin(String blePassword) {
  this->blePassword = blePassword;
  if (debugMode == CCA_DEBUG_OFF) Serial.begin(115200);
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
  if (!blePassword.length() == 0) {
    Serial.println("Passwort aktiv: AUTH-Befehl erforderlich.");
  }
  Serial.println("Warte auf Verbindung...\n");
}

void CCARemoteBLE::handle() {
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
  return deviceConnected;
}

void CCARemoteBLE::sendInternal(String key, String value) {
  if (deviceConnected && authenticated && pDisplayChar != nullptr) {
    String msg = key + ":" + value;
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
                           uint32_t baudRate)
  : CCARemote(name, prefix),
    _serial(nullptr),
    _rxPin(rxPin), _txPin(txPin), _baudRate(baudRate),
    _connected(false), _authenticated(false),
    _lastByteTime(0)
{}

CCARemoteBLE::~CCARemoteBLE() {
  delete _serial;
}

void CCARemoteBLE::begin(String blePassword) {
  _blePassword = blePassword;
  if (debugMode == CCA_DEBUG_OFF) Serial.begin(9600);
  Serial.println("\nCCA Remote startet (BLE/HM-10)...");
  Serial.println("Geraetename: " + deviceName);

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

  Serial.println("HM-10 bereit! (RX=" + String(_rxPin) + ", TX=" + String(_txPin) + ")");
  if (!blePassword.length() == 0) {
    Serial.println("Passwort aktiv: AUTH-Befehl erforderlich.");
  }
  Serial.println("Warte auf Verbindung...\n");
}

void CCARemoteBLE::handle() {
  if (_pendingResync && _connected && _authenticated) {
    _pendingResync = false;
    _resyncDisplay();
  }

  // Alle verfuegbaren Bytes einlesen
  while (_serial->available()) {
    _rxBuffer += (char)_serial->read();
    _lastByteTime = millis();
  }

  // Nach 20 ms Pause ohne neue Bytes → Paket vollstaendig
  if (_rxBuffer.length() > 0 && (millis() - _lastByteTime) >= 20) {
    String data = _rxBuffer;
    _rxBuffer   = "";
    data.trim();
    if (data.length() > 0) {
      _processRx(data);
    }
  }

}

bool CCARemoteBLE::isConnected() {
  return _connected;
}

void CCARemoteBLE::sendInternal(String key, String value) {
  if (_connected && _authenticated) {
    _sendRaw(key + ":" + value);
  }
}

void CCARemoteBLE::_processRx(String data) {
  // Verbindungs-Events (je nach HM-10-Firmware-Version)
  if (data.indexOf("OK+CONN") >= 0 || data.indexOf("+CONNECTED") >= 0) {
    _connected     = true;
    _authenticated = _blePassword.length() == 0;
    _pendingResync = true;
    if (debugMode != CCA_DEBUG_OFF) Serial.println("[CCA] Verbindung hergestellt");
    return;
  }
  if (data.indexOf("OK+LOST") >= 0 || data.indexOf("OK+LOSTA") >= 0 ||
      data.indexOf("+DISCONNECTED") >= 0 || data.indexOf("DISCONNECT") >= 0) {
    _connected     = false;
    _authenticated = false;
    _lastByteTime  = 0;
    Serial.println("[CCA] Verbindung getrennt");
    return;
  }

  // AT-Antworten des Moduls ignorieren
  if (data.startsWith("OK+")) return;

  // Viele HM-10 Klone senden kein Verbindungs-Event → beim ersten Datenpaket implizit verbinden
  if (!_connected) {
    _connected     = true;
    _authenticated = _blePassword.length() == 0;
    _pendingResync = true;
    if (debugMode != CCA_DEBUG_OFF) Serial.println("[CCA] Verbindung hergestellt (implizit)");
  }

  // Authentifizierung pruefen, falls Passwort gesetzt
  if (!_blePassword.length() == 0 && !_authenticated) {
    if (data == "AUTH:" + _blePassword) {
      _authenticated = true;
      _pendingResync = true;
      Serial.println("BLE Authentifizierung erfolgreich!");
      _sendRaw("AUTH:OK");
    } else {
      Serial.println("BLE Authentifizierung fehlgeschlagen!");
      _sendRaw("AUTH:FAIL");
      // HM-10 kann nicht aktiv trennen – als getrennt markieren
      // damit keine weiteren Befehle verarbeitet werden
      _connected = false;
    }
    return;
  }

  if (!_authenticated) return;

  // Auth-Handshake der App ohne Passwort ignorieren
  if (data == "AUTH" || data.startsWith("AUTH:")) return;

  processCommand(data);
}

void CCARemoteBLE::_sendRaw(String msg) {
  if (_serial) _serial->print(msg);
}

#endif // ESP32
