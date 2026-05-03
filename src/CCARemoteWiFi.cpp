/*
 * CCARemoteWiFi.cpp – WiFi Access Point Implementation
 *
 * Based on the diploma thesis by L. Eder and E. Duyar (HTL Anichstraße)
 * Extended by A. Eckhart with kind permission of the original authors.
 *
 * Version: 1.0.0 | 2026-05-03 | MIT – see LICENSE
 */

#include "CCARemoteWiFi.h"

CCARemoteWiFi::CCARemoteWiFi(String name, String prefix) : CCARemote(name, prefix) {
  webServer   = nullptr;
  wifiEnabled = false;
}

CCARemoteWiFi::~CCARemoteWiFi() {
  if (webServer != nullptr) {
    webServer->stop();
    delete webServer;
  }
}

void CCARemoteWiFi::begin(String wifiPassword) {
  if (!debugEnabled) Serial.begin(115200);
  Serial.println("\nCCA Remote startet (WiFi)...");
  Serial.println("Geraetename: " + deviceName);

  bool success;
  if (wifiPassword.isEmpty()) {
    success = WiFi.softAP(deviceName.c_str());
  } else {
    success = WiFi.softAP(deviceName.c_str(), wifiPassword.c_str());
  }

  if (!success) {
    Serial.println("WiFi AP Start fehlgeschlagen!");
    return;
  }

  wifiEnabled = true;
  Serial.print("WiFi AP: ");
  Serial.println(deviceName);
  Serial.print("IP-Adresse: ");
  Serial.println(WiFi.softAPIP());
  if (!wifiPassword.isEmpty()) {
    Serial.print("Passwort: ");
    Serial.println(wifiPassword);
  }

  webServer = new WebServer(80);
  webServer->on("/",        HTTP_GET,  [this]() { this->handleRoot();    });
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
}

bool CCARemoteWiFi::isConnected() {
  return wifiEnabled && (WiFi.softAPgetStationNum() > 0);
}

void CCARemoteWiFi::sendInternal(String key, String value) {
  bool changed = (displayValues.find(key) == displayValues.end()) ||
                 (displayValues[key] != value);
  displayValues[key] = value;
  if (changed) {
    Serial.println("Display: " + key + " = " + value);
  }
}

void CCARemoteWiFi::handleRoot() {
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
  processCommand(webServer->arg("plain"));
  webServer->send(200, "application/json", "{\"status\":\"ok\"}");
}

void CCARemoteWiFi::handleDisplay() {
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