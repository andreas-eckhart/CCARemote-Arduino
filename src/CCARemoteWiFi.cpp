/*
 * CCARemoteWiFi.cpp – WiFi Access Point Implementation
 *
 * Platform detection (automatic):
 *   ESP32   → WiFi.h
 *   ESP8266 → ESP8266WiFi.h
 *   AVR     → not supported (file compiles empty)
 *
 * Based on the diploma thesis by L. Eder and E. Duyar (HTL Anichstraße)
 * Extended by A. Eckhart with kind permission of the original authors.
 *
 * MIT – see LICENSE
 */

#include "CCARemoteWiFi.h"

#if !defined(__AVR__)

CCARemoteWiFi::CCARemoteWiFi(String name, String prefix,
                              String password, uint16_t port,
                              CCADebugMode debugLevel, unsigned long baudRate)
  : CCARemote(name, prefix, debugLevel, baudRate)
{
  _password     = password;
  _port         = port;
  wifiEnabled   = false;
  _wasConnected = false;
  _tcpServer    = nullptr;
  _tcpBuf       = "";
}

CCARemoteWiFi::~CCARemoteWiFi() {
  if (_tcpServer != nullptr) {
    _tcpServer->stop();
    delete _tcpServer;
  }
}

void CCARemoteWiFi::begin() {
  Serial.begin(_serialBaudRate);
#if defined(ESP8266)
  if (debugMode != CCA_DEBUG_OFF) delay(3000);
#endif
  Serial.println("\nCCA Remote startet (WiFi)...");
  if (debugMode == CCA_DEBUG_ALL)
    Serial.println("[CCA] Lib: " CCA_LIB_VERSION "  |  Protokoll: " CCA_PROTOCOL_VERSION);
  Serial.println("Geraetename: " + deviceName);

#if defined(ESP8266)
  WiFi.persistent(false);
  WiFi.mode(WIFI_OFF);
  delay(100);
  WiFi.mode(WIFI_AP);
  delay(100);
#endif

  if (_password.length() > 0 && _password.length() < 8) {
    while (true) {
      Serial.println("[CCA] FEHLER: WiFi-Passwort muss mindestens 8 Zeichen lang sein!");
      delay(2000);
    }
  }

  bool success;
  if (_password.length() == 0) {
    success = WiFi.softAP(deviceName.c_str());
  } else {
    success = WiFi.softAP(deviceName.c_str(), _password.c_str());
  }

#if defined(ESP8266)
  delay(100);
#endif

  if (!success) {
    while (true) {
      Serial.println("[CCA] FEHLER: WiFi AP Start fehlgeschlagen!");
      delay(2000);
    }
  }

  wifiEnabled = true;
  Serial.print("WiFi AP: ");
  Serial.println(deviceName);
  if (_password.length() > 0) {
    Serial.print("WiFi Passwort: ");
    if (debugMode == CCA_DEBUG_ALL) {
      Serial.println(_password);
    } else {
      for (unsigned int i = 0; i < _password.length(); i++) Serial.print('*');
      Serial.println();
    }
  }
  Serial.print("IP-Adresse: ");
  Serial.println(WiFi.softAPIP());

  _tcpServer = new WiFiServer(_port);
  _tcpServer->begin();
  Serial.println("TCP Server laeuft auf Port " + String(_port));
  Serial.println("CCA Remote bereit!\n");
}

void CCARemoteWiFi::handle() {
  _checkWatchdogs();
  if (!wifiEnabled) return;

  // Neuen TCP-Client annehmen
  if (_tcpServer && !_tcpClient.connected()) {
    if (_tcpServer->hasClient()) {
      _tcpClient = _tcpServer->accept();
      _tcpBuf    = "";
      for (auto const& p : displayValues)
        _tcpClient.print(p.first + ":" + p.second + "\n");
    }
  }

  // Eingehende Zeilen lesen
  if (_tcpClient.connected()) {
    while (_tcpClient.available()) {
      char c = _tcpClient.read();
      if (c == '\n') {
        _tcpBuf.trim();
        if (_tcpBuf == "disconnect:1") {
          _tcpDisconnect();
          break;
        } else if (_tcpBuf.length() > 0) {
          processCommand(_tcpBuf);
        }
        _tcpBuf = "";
      } else {
        _tcpBuf += c;
      }
    }
  }

  bool nowConnected = isConnected();
  if (nowConnected != _wasConnected) {
    _wasConnected = nowConnected;
    if (debugMode != CCA_DEBUG_OFF) {
      Serial.println(nowConnected ? "[CCA] Verbindung hergestellt" : "[CCA] Verbindung getrennt");
    }
  }

#if defined(ESP8266)
  static unsigned long startTime = millis();
  static unsigned long lastPrint = 0;
  if (wifiEnabled && millis() - startTime < 10000) {
    if (millis() - lastPrint >= 2000) {
      lastPrint = millis();
      Serial.println("[CCA] WiFi AP: " + deviceName + " | IP: " + WiFi.softAPIP().toString());
    }
  }
#endif
}

bool CCARemoteWiFi::isConnected() {
  if (!wifiEnabled) return false;
  return _tcpClient.connected();
}

void CCARemoteWiFi::_tcpDisconnect() {
  _tcpClient.stop();
  _tcpBuf = "";
}

void CCARemoteWiFi::sendInternal(String key, String value) {
  displayValues[key] = value;
  if (_tcpClient.connected())
    _tcpClient.print(key + ":" + value + "\n");
}

#endif // !__AVR__
