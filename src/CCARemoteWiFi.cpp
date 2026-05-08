/*
 * CCARemoteWiFi.cpp – WiFi Access Point Implementation
 *
 * Based on the diploma thesis by L. Eder and E. Duyar (HTL Anichstraße)
 * Extended by A. Eckhart with kind permission of the original authors.
 *
 * Version: 1.2.0 | 2026-05-08 | MIT – see LICENSE
 */

#include "CCARemoteWiFi.h"

#if !defined(__AVR__)

CCARemoteWiFi::CCARemoteWiFi(String name, String prefix) : CCARemote(name, prefix) {
  webServer      = nullptr;
  wifiEnabled    = false;
  _lastRequestMs = 0;
  _wasConnected  = false;
}

CCARemoteWiFi::~CCARemoteWiFi() {
  if (webServer != nullptr) {
    webServer->stop();
    delete webServer;
  }
}

void CCARemoteWiFi::begin(String wifiPassword) {
  if (debugMode == CCA_DEBUG_OFF) {
    Serial.begin(115200);
#if defined(ESP8266)
    delay(3000);
#endif
  }
  Serial.println("\nCCA Remote startet (WiFi)...");
  Serial.println("Geraetename: " + deviceName);

#if defined(ESP8266)
  WiFi.persistent(false);
  WiFi.mode(WIFI_OFF);
  delay(100);
  WiFi.mode(WIFI_AP);
  delay(100);
#endif

  if (wifiPassword.length() > 0 && wifiPassword.length() < 8) {
    while (true) {
      Serial.println("[CCA] FEHLER: WiFi-Passwort muss mindestens 8 Zeichen lang sein!");
      delay(2000);
    }
  }

  bool success;
  if (wifiPassword.length() == 0) {
    success = WiFi.softAP(deviceName.c_str());
  } else {
    success = WiFi.softAP(deviceName.c_str(), wifiPassword.c_str());
  }

#if defined(ESP8266)
  delay(100);  // ESP8266: kurz warten bis IP vergeben ist
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
  Serial.print("IP-Adresse: ");
  Serial.println(WiFi.softAPIP());
  if (wifiPassword.length() > 0) {
    Serial.print("Passwort: ");
    Serial.println(wifiPassword);
  }

  webServer = new CCAWebServer(80);
  webServer->on("/",        HTTP_GET,  [this]() { this->handleRoot();    });
  webServer->on("/status",  HTTP_GET,  [this]() { this->handleStatus();  });
  webServer->on("/command", HTTP_POST, [this]() { this->handleCommand(); });
  webServer->on("/display", HTTP_GET,  [this]() { this->handleDisplay(); });
  webServer->begin();

  Serial.println("HTTP Server laeuft auf Port 80");
  Serial.println("CCA Remote bereit!\n");
}

void CCARemoteWiFi::handle() {
  if (wifiEnabled && webServer != nullptr) {
    webServer->handleClient();
  }

  bool nowConnected = isConnected();
  if (nowConnected != _wasConnected) {
    _wasConnected = nowConnected;
    if (debugMode != CCA_DEBUG_OFF) {
      Serial.println(nowConnected ? "[CCA] Verbindung hergestellt" : "[CCA] Verbindung getrennt");
    }
  }

#if defined(ESP8266)
  // Startinfo 10 s lang alle 2 s wiederholen – Serial Monitor öffnet sich oft erst nach dem Upload
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
  if (WiFi.softAPgetStationNum() == 0) return false;
  // Kein HTTP-Request seit CONNECTION_TIMEOUT_MS, oder expliziter Disconnect → false
  if (_lastRequestMs == 0 || (millis() - _lastRequestMs) > CONNECTION_TIMEOUT_MS) return false;
  return true;
}

void CCARemoteWiFi::_updateLastRequest() {
  _lastRequestMs = millis();
}

void CCARemoteWiFi::sendInternal(String key, String value) {
  displayValues[key] = value;
}

void CCARemoteWiFi::handleStatus() {
  _updateLastRequest();
  String json = "{\"type\":\"CCARemote\",\"device\":\"" + deviceName + "\"}";
  webServer->send(200, "application/json", json);
}

void CCARemoteWiFi::handleRoot() {
  _updateLastRequest();
  String html = "<!DOCTYPE html><html><head><meta charset='UTF-8'>";
  html += "<title>" + deviceName + "</title></head><body>";
  html += "<h1>" + deviceName + "</h1>";
  html += "<p>CCA Remote WiFi laeuft</p>";
  html += "<p>Verbundene Geraete: " + String(WiFi.softAPgetStationNum()) + "</p>";
  html += "<p><strong>POST /command</strong> - Body: befehl:wert</p>";
  html += "<p><strong>GET /display</strong> - JSON mit Display-Werten</p>";
  html += "</body></html>";
  webServer->send(200, "text/html", html);
}

void CCARemoteWiFi::handleCommand() {
  if (!webServer->hasArg("plain")) {
    webServer->send(400, "application/json", "{\"error\":\"no body\"}");
    return;
  }
  String body = webServer->arg("plain");
  // App-seitiger Disconnect – Timeout sofort auslösen
  if (body == "disconnect:1") {
    _lastRequestMs = 0;
    webServer->send(200, "application/json", "{\"status\":\"ok\"}");
    return;
  }
  _updateLastRequest();
  processCommand(body);
  webServer->send(200, "application/json", "{\"status\":\"ok\"}");
}

void CCARemoteWiFi::handleDisplay() {
  _updateLastRequest();
  String json = "{";
  bool first = true;
  for (auto const& pair : displayValues) {
    if (!first) json += ",";
    json += "\"" + pair.first + "\":\"" + pair.second + "\"";
    first = false;
  }
  json += "}";
  webServer->send(200, "application/json", json);
}

#endif // !__AVR__