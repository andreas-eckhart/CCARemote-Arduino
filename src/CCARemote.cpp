/*
 * CCARemote.cpp – Abstract Base Class Implementation
 *
 * Based on the diploma thesis by L. Eder and E. Duyar (HTL Anichstraße)
 * Extended by A. Eckhart with kind permission of the original authors.
 *
 * MIT – see LICENSE
 */

#include "CCARemoteBase.h"

// ================================================================
#if defined(__AVR__)
// ================================================================
//  AVR (Uno/Nano)
// ================================================================

CCARemote::CCARemote(String name, String prefix, CCADebugMode debugLevel, unsigned long baudRate) {
  deviceName       = prefix + name;
  commandReceived  = false;
  lastCommand      = "";
  debugMode        = debugLevel;
  _serialBaudRate  = baudRate;
  _cmdCount        = 0;
  _cmdVCount       = 0;
  _recvCount       = 0;
  _colorRecvCount  = 0;
  _displayCount    = 0;
  _pendingResync   = false;
  _watchdogCount   = 0;
  // Handshake-Keys vorinitialisieren (werden bei jedem Connect gesendet)
  _display[_displayCount++] = { "protocol",   CCA_PROTOCOL_VERSION };
  _display[_displayCount++] = { "platform",   CCA_PLATFORM         };
  _display[_displayCount++] = { "libVersion", CCA_LIB_VERSION      };
}

void CCARemote::onCommand(String cmd, void (*callback)()) {
  if (_cmdCount < CCA_MAX_CALLBACKS) {
    _cmds[_cmdCount++] = { cmd, callback };
    Serial.print(F("Befehl registriert: ")); Serial.println(cmd);
  }
}

void CCARemote::onCommand(String cmd, void (*callback)(String)) {
  if (_cmdVCount < CCA_MAX_CALLBACKS) {
    _cmdsV[_cmdVCount++] = { cmd, callback };
    Serial.print(F("Befehl registriert: ")); Serial.print(cmd); Serial.println(F(" (mit Wert)"));
  }
}

void CCARemote::receive(String cmd, int& var) {
  if (_recvCount < CCA_MAX_RECEIVERS) {
    _recv[_recvCount++] = { cmd, _CCARecv::INT_T, &var };
    Serial.print(F("Variable gebunden: ")); Serial.print(cmd); Serial.println(F(" (int)"));
  }
}

void CCARemote::receive(String cmd, bool& var) {
  if (_recvCount < CCA_MAX_RECEIVERS) {
    _recv[_recvCount++] = { cmd, _CCARecv::BOOL_T, &var };
    Serial.print(F("Variable gebunden: ")); Serial.print(cmd); Serial.println(F(" (bool)"));
  }
}

void CCARemote::receive(String cmd, float& var) {
  if (_recvCount < CCA_MAX_RECEIVERS) {
    _recv[_recvCount++] = { cmd, _CCARecv::FLOAT_T, &var };
    Serial.print(F("Variable gebunden: ")); Serial.print(cmd); Serial.println(F(" (float)"));
  }
}

void CCARemote::receive(String cmd, String& var) {
  if (_recvCount < CCA_MAX_RECEIVERS) {
    _recv[_recvCount++] = { cmd, _CCARecv::STRING_T, &var };
    Serial.print(F("Variable gebunden: ")); Serial.print(cmd); Serial.println(F(" (String)"));
  }
}

void CCARemote::receiveColor(String cmd, int& r, int& g, int& b) {
  if (_colorRecvCount < CCA_MAX_COLOR) {
    _colorRecv[_colorRecvCount++] = { cmd, &r, &g, &b };
    Serial.print(F("Farbe gebunden: ")); Serial.print(cmd); Serial.println(F(" (r,g,b)"));
  }
}

void CCARemote::processCommand(String cmd) {
  int start = 0;
  while (start <= (int)cmd.length()) {
    int commaPos = cmd.indexOf(',', start);
    String part = (commaPos < 0) ? cmd.substring(start) : cmd.substring(start, commaPos);
    part.trim();

    if (part.length() > 0) {
      int colonPos = part.indexOf(':');
      if (colonPos > 0) {
        String key   = part.substring(0, colonPos);
        String value = part.substring(colonPos + 1);

        // Color-Bindings prüfen
        bool found = false;
        for (uint8_t i = 0; i < _colorRecvCount; i++) {
          if (_colorRecv[i].key == key) {
            int s1 = value.indexOf(';');
            int s2 = value.indexOf(';', s1 + 1);
            if (s1 > 0 && s2 > s1) {
              *_colorRecv[i].r = value.substring(0, s1).toInt();
              *_colorRecv[i].g = value.substring(s1 + 1, s2).toInt();
              *_colorRecv[i].b = value.substring(s2 + 1).toInt();
            }
            if (debugMode & CCA_DEBUG_IN) {
              Serial.print(F("[CCA] IN  ")); Serial.print(key);
              Serial.print(F(" = R:")); Serial.print(*_colorRecv[i].r);
              Serial.print(F(" G:"));   Serial.print(*_colorRecv[i].g);
              Serial.print(F(" B:"));   Serial.println(*_colorRecv[i].b);
            }
            found = true;
            break;
          }
        }
        // Variable-Bindings prüfen
        if (!found) {
          for (uint8_t i = 0; i < _recvCount; i++) {
            if (_recv[i].key == key) {
              if (debugMode & CCA_DEBUG_IN) {
                Serial.print(F("[CCA] IN  ")); Serial.print(key);
                Serial.print(F(" = "));        Serial.println(value);
              }
              switch (_recv[i].type) {
                case _CCARecv::INT_T:
                  *((int*)_recv[i].ptr) = value.toInt(); break;
                case _CCARecv::BOOL_T:
                  *((bool*)_recv[i].ptr) = (value == "1" || value == "true" || value == "on"); break;
                case _CCARecv::FLOAT_T:
                  *((float*)_recv[i].ptr) = value.toFloat(); break;
                case _CCARecv::STRING_T:
                  *((String*)_recv[i].ptr) = value; break;
              }
              found = true;
              break;
            }
          }
        }
        // Callback mit Wert prüfen
        if (!found) {
          for (uint8_t i = 0; i < _cmdVCount; i++) {
            if (_cmdsV[i].key == key) {
              if (debugMode & CCA_DEBUG_IN) {
                Serial.print(F("[CCA] IN  ")); Serial.print(key);
                Serial.print(F(" = "));        Serial.println(value);
              }
              _cmdsV[i].fn(value);
              found = true;
              break;
            }
          }
        }
        if (found) {
          for (uint8_t w = 0; w < _watchdogCount; w++) {
            if (_watchdogList[w].key == key) { _watchdogList[w].lastMs = millis(); break; }
          }
        } else {
          Serial.print(F("Unbekannter Befehl: ")); Serial.println(key);
        }
      } else {
        bool found = false;
        for (uint8_t i = 0; i < _cmdCount; i++) {
          if (_cmds[i].key == part) {
            if (debugMode & CCA_DEBUG_IN) {
              Serial.print(F("[CCA] IN  ")); Serial.print(part); Serial.println(F(" (kein Wert)"));
            }
            _cmds[i].fn();
            found = true;
            break;
          }
        }
        if (!found) { Serial.print(F("Unbekannter Befehl: ")); Serial.println(part); }
      }
    }

    if (commaPos < 0) break;
    start = commaPos + 1;
  }
}

void CCARemote::watchdog(String cmd, unsigned long timeoutMs) {
  for (uint8_t i = 0; i < _watchdogCount; i++) {
    if (_watchdogList[i].key == cmd) { _watchdogList[i].timeoutMs = timeoutMs; _watchdogList[i].lastMs = millis(); return; }
  }
  if (_watchdogCount < CCA_MAX_RECEIVERS) {
    _watchdogList[_watchdogCount].key       = cmd;
    _watchdogList[_watchdogCount].timeoutMs = timeoutMs;
    _watchdogList[_watchdogCount].lastMs    = millis();
    _watchdogCount++;
  }
}
void CCARemote::_checkWatchdogs() {
  unsigned long now = millis();
  for (uint8_t i = 0; i < _watchdogCount; i++) {
    if (now - _watchdogList[i].lastMs >= _watchdogList[i].timeoutMs) {
      _watchdogList[i].lastMs = now;
      processCommand(_watchdogList[i].key + ":0");
    }
  }
}

// ================================================================
#else
// ================================================================
//  ESP32 / andere – std::function + std::map
// ================================================================

CCARemote::CCARemote(String name, String prefix, CCADebugMode debugLevel, unsigned long baudRate) {
  deviceName      = prefix + name;
  commandReceived = false;
  lastCommand     = "";
  debugMode       = debugLevel;
  _serialBaudRate = baudRate;
  _pendingResync  = false;
  // Handshake-Keys vorinitialisieren (werden bei jedem Connect gesendet)
  displayValues["protocol"]   = CCA_PROTOCOL_VERSION;
  displayValues["platform"]   = CCA_PLATFORM;
  displayValues["libVersion"] = CCA_LIB_VERSION;
}

void CCARemote::onCommand(String cmd, std::function<void()> callback) {
  commands[cmd] = callback;
  Serial.println("Befehl registriert: " + cmd);
}

void CCARemote::onCommand(String cmd, std::function<void(String)> callback) {
  commandsWithValue[cmd] = callback;
  Serial.println("Befehl registriert: " + cmd + " (mit Wert)");
}

void CCARemote::receive(String cmd, int& var) {
  commandsWithValue[cmd] = [this, cmd, &var](String value) {
    var = value.toInt();
    if (debugMode & CCA_DEBUG_IN) Serial.println("[CCA] IN  " + cmd + " = " + value);
  };
  Serial.println("Variable gebunden: " + cmd + " (int)");
}

void CCARemote::receive(String cmd, bool& var) {
  commandsWithValue[cmd] = [this, cmd, &var](String value) {
    var = (value == "1" || value == "true" || value == "on");
    if (debugMode & CCA_DEBUG_IN) Serial.println("[CCA] IN  " + cmd + " = " + value);
  };
  Serial.println("Variable gebunden: " + cmd + " (bool)");
}

void CCARemote::receive(String cmd, float& var) {
  commandsWithValue[cmd] = [this, cmd, &var](String value) {
    var = value.toFloat();
    if (debugMode & CCA_DEBUG_IN) Serial.println("[CCA] IN  " + cmd + " = " + value);
  };
  Serial.println("Variable gebunden: " + cmd + " (float)");
}

void CCARemote::receive(String cmd, String& var) {
  commandsWithValue[cmd] = [this, cmd, &var](String value) {
    var = value;
    if (debugMode & CCA_DEBUG_IN) Serial.println("[CCA] IN  " + cmd + " = " + value);
  };
  Serial.println("Variable gebunden: " + cmd + " (String)");
}

void CCARemote::receiveColor(String cmd, int& r, int& g, int& b) {
  commandsWithValue[cmd] = [this, cmd, &r, &g, &b](String value) {
    int s1 = value.indexOf(';');
    int s2 = value.indexOf(';', s1 + 1);
    if (s1 > 0 && s2 > s1) {
      r = value.substring(0, s1).toInt();
      g = value.substring(s1 + 1, s2).toInt();
      b = value.substring(s2 + 1).toInt();
    }
    if (debugMode & CCA_DEBUG_IN)
      Serial.println("[CCA] IN  " + cmd + " = R:" + r + " G:" + g + " B:" + b);
  };
  Serial.println("Farbe gebunden: " + cmd + " (r,g,b)");
}

void CCARemote::processCommand(String cmd) {
  int start = 0;
  while (start <= (int)cmd.length()) {
    int commaPos = cmd.indexOf(',', start);
    String part = (commaPos < 0) ? cmd.substring(start) : cmd.substring(start, commaPos);
    part.trim();

    if (part.length() > 0) {
      int colonPos = part.indexOf(':');
      if (colonPos > 0) {
        String command = part.substring(0, colonPos);
        String value   = part.substring(colonPos + 1);
        if (commandsWithValue.count(command) > 0) {
          commandsWithValue[command](value);
          if (_watchdogLast.count(command)) _watchdogLast[command] = millis();
        } else {
          Serial.println("Unbekannter Befehl: " + command);
        }
      } else {
        if (commands.count(part) > 0) {
          if (debugMode & CCA_DEBUG_IN) Serial.println("[CCA] IN  " + part + " (kein Wert)");
          commands[part]();
          if (_watchdogLast.count(part)) _watchdogLast[part] = millis();
        } else {
          Serial.println("Unbekannter Befehl: " + part);
        }
      }
    }

    if (commaPos < 0) break;
    start = commaPos + 1;
  }
}

void CCARemote::watchdog(String cmd, unsigned long timeoutMs) {
  _watchdogTimeouts[cmd] = timeoutMs;
  _watchdogLast[cmd]     = millis();
}
void CCARemote::_checkWatchdogs() {
  unsigned long now = millis();
  for (auto& kv : _watchdogTimeouts) {
    if (now - _watchdogLast[kv.first] >= kv.second) {
      _watchdogLast[kv.first] = now;
      processCommand(kv.first + ":0");
    }
  }
}

#endif // __AVR__

// ================================================================
//  Gemeinsame Methoden (beide Plattformen)
// ================================================================

void CCARemote::_resyncDisplay() {
#if defined(__AVR__)
  for (uint8_t i = 0; i < _displayCount; i++) {
    sendInternal(_display[i].key, _display[i].value);
  }
#else
  for (auto const& pair : displayValues) {
    sendInternal(pair.first, pair.second);
  }
#endif
}

void CCARemote::debug(CCADebugMode mode, unsigned long baudRate) {
  debugMode = mode;
  if (mode != CCA_DEBUG_OFF) {
    Serial.begin(baudRate);
#if defined(ESP8266)
    delay(3000);  // ESP8266: warten bis Serial Monitor nach Upload/Power-On bereit ist
#endif
  }
  if (mode == CCA_DEBUG_OFF)  Serial.println("[CCA] Debug-Modus deaktiviert");
  if (mode == CCA_DEBUG_IN)   Serial.println("[CCA] Debug-Modus: nur IN");
  if (mode == CCA_DEBUG_OUT)  Serial.println("[CCA] Debug-Modus: nur OUT");
  if (mode == CCA_DEBUG_ALL)  Serial.println("[CCA] Debug-Modus: IN + OUT");
}

void CCARemote::_sendIfChanged(String key, String value) {
#if defined(__AVR__)
  bool found = false;
  for (uint8_t i = 0; i < _displayCount; i++) {
    if (_display[i].key == key) {
      if (_display[i].value == value) return;
      _display[i].value = value;
      found = true;
      break;
    }
  }
  if (!found && _displayCount < CCA_MAX_DISPLAY) {
    _display[_displayCount].key   = key;
    _display[_displayCount].value = value;
    _displayCount++;
  }
#else
  auto it = displayValues.find(key);
  if (it != displayValues.end() && it->second == value) return;
  displayValues[key] = value;
#endif
  if (debugMode & CCA_DEBUG_OUT) {
    Serial.print(F("[CCA] OUT "));
    Serial.print(key);
    if (value.length() > 0) { Serial.print(F(" = ")); Serial.println(value); }
    else                       Serial.println();
  }
  sendInternal(key, value);
}

void CCARemote::send(String message) {
  int colonPos = message.indexOf(':');
  if (colonPos > 0)
    _sendIfChanged(message.substring(0, colonPos), message.substring(colonPos + 1));
  else
    _sendIfChanged(message, "");
}

void CCARemote::send(String key, String value) {
  _sendIfChanged(key, value);
}

void CCARemote::send(String key, int value) {
  _sendIfChanged(key, String(value));
}

void CCARemote::send(String key, float value) {
  _sendIfChanged(key, String(value, 1));
}

void CCARemote::send(String key, float value, int decimals) {
  _sendIfChanged(key, String(value, decimals));
}
