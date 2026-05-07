/*
 * CCARemoteBLE.cpp – Bluetooth Low Energy (BLE) Implementation
 *
 * Based on the diploma thesis by L. Eder and E. Duyar (HTL Anichstraße)
 * Extended by A. Eckhart with kind permission of the original authors.
 *
 * Version: 1.0.0 | 2026-05-03 | MIT – see LICENSE
 */

#include "CCARemoteBLE.h"

const char* CCARemoteBLE::SERVICE_UUID = "4fafc201-1fb5-459e-8fcc-c5c9c331914b";
const char* CCARemoteBLE::CONTROL_UUID = "cba1d466-344c-4be3-ab3f-189f80dd7518"; // App → Arduino
const char* CCARemoteBLE::DISPLAY_UUID = "d9a98a3e-7c1f-4b2a-9e8f-6d2c3a1b5e7f"; // Arduino → App

class CCARemoteBLE::ServerCallbacks : public BLEServerCallbacks {
  CCARemoteBLE* parent;
public:
  ServerCallbacks(CCARemoteBLE* p) : parent(p) {}
  void onConnect(BLEServer*) override {
    parent->deviceConnected = true;
    parent->authenticated   = parent->blePassword.isEmpty();
    Serial.println("Geraet verbunden!");
  }
  void onDisconnect(BLEServer*) override {
    parent->deviceConnected = false;
    parent->authenticated   = false;
    Serial.println("Geraet getrennt!");
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

    // Authentifizierung prüfen, falls Passwort gesetzt
    if (!parent->blePassword.isEmpty() && !parent->authenticated) {
      if (value == "AUTH:" + parent->blePassword) {
        parent->authenticated = true;
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

  // Befehlscharakteristik: App → Arduino (nur WRITE)
  pControlChar = pService->createCharacteristic(
    CONTROL_UUID,
    BLECharacteristic::PROPERTY_WRITE |
    BLECharacteristic::PROPERTY_WRITE_NR
  );
  pControlChar->setCallbacks(new ControlCallbacks(this));

  // Display-Charakteristik: Arduino → App (nur NOTIFY)
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
  if (!blePassword.isEmpty()) {
    Serial.println("Passwort aktiv: AUTH-Befehl erforderlich.");
  }
  Serial.println("Warte auf Verbindung...\n");
}

void CCARemoteBLE::handle() {
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