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
//  Einheitliche Implementierung für alle Plattformen (flat arrays)
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
  _watchdogFired   = 0;
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

void CCARemote::processCommand(const char* cmd) {
  const char* p = cmd;
  while (*p) {
    const char* comma   = strchr(p, ',');
    const char* partEnd = comma ? comma : (p + strlen(p));

    while (p < partEnd && *p == ' ') p++;
    while (partEnd > p && *(partEnd - 1) == ' ') partEnd--;

    uint8_t partLen = (uint8_t)(partEnd - p);
    if (partLen == 0) { if (!comma) break; p = comma + 1; continue; }

    // Doppelpunkt suchen
    const char* colon = nullptr;
    for (const char* c = p; c < partEnd; c++) { if (*c == ':') { colon = c; break; } }

    if (colon && colon > p) {
      const char* keyStr = p;
      uint8_t     keyLen = (uint8_t)(colon - p);
      const char* valStr = colon + 1;
      uint8_t     valLen = (uint8_t)(partEnd - valStr);
      bool found = false;

      // Color-Bindings
      for (uint8_t i = 0; i < _colorRecvCount; i++) {
        if (_colorRecv[i].key.length() == keyLen &&
            memcmp(_colorRecv[i].key.c_str(), keyStr, keyLen) == 0) {
          const char* s1 = (const char*)memchr(valStr, ';', valLen);
          if (s1 && s1 > valStr) {
            const char* s2 = (const char*)memchr(s1 + 1, ';', valLen - (uint8_t)(s1 + 1 - valStr));
            if (s2) {
              *_colorRecv[i].r = atoi(valStr);
              *_colorRecv[i].g = atoi(s1 + 1);
              *_colorRecv[i].b = atoi(s2 + 1);
            }
          }
          if (debugMode & CCA_DEBUG_IN) {
            Serial.print(F("[CCA] IN  "));
            Serial.write(reinterpret_cast<const uint8_t*>(keyStr), keyLen);
            Serial.print(F(" = R:")); Serial.print(*_colorRecv[i].r);
            Serial.print(F(" G:"));   Serial.print(*_colorRecv[i].g);
            Serial.print(F(" B:"));   Serial.println(*_colorRecv[i].b);
          }
          found = true; break;
        }
      }

      // Variable-Bindings
      if (!found) {
        for (uint8_t i = 0; i < _recvCount; i++) {
          if (_recv[i].key.length() == keyLen &&
              memcmp(_recv[i].key.c_str(), keyStr, keyLen) == 0) {
            if (debugMode & CCA_DEBUG_IN) {
              Serial.print(F("[CCA] IN  "));
              Serial.write(reinterpret_cast<const uint8_t*>(keyStr), keyLen);
              Serial.print(F(" = "));
              Serial.write(reinterpret_cast<const uint8_t*>(valStr), valLen);
              Serial.println();
            }
            switch (_recv[i].type) {
              case _CCARecv::INT_T:
                *((int*)_recv[i].ptr) = atoi(valStr); break;
              case _CCARecv::BOOL_T:
                *((bool*)_recv[i].ptr) =
                  (valLen == 1 && valStr[0] == '1') ||
                  (valLen == 4 && memcmp(valStr, "true", 4) == 0) ||
                  (valLen == 2 && memcmp(valStr, "on",   2) == 0); break;
              case _CCARecv::FLOAT_T:
                *((float*)_recv[i].ptr) = atof(valStr); break;
              case _CCARecv::STRING_T:
                *((String*)_recv[i].ptr) = String(valStr).substring(0, valLen); break;
            }
            found = true; break;
          }
        }
      }

      // Callback mit Wert
      if (!found) {
        for (uint8_t i = 0; i < _cmdVCount; i++) {
          if (_cmdsV[i].key.length() == keyLen &&
              memcmp(_cmdsV[i].key.c_str(), keyStr, keyLen) == 0) {
            if (debugMode & CCA_DEBUG_IN) {
              Serial.print(F("[CCA] IN  "));
              Serial.write(reinterpret_cast<const uint8_t*>(keyStr), keyLen);
              Serial.print(F(" = "));
              Serial.write(reinterpret_cast<const uint8_t*>(valStr), valLen);
              Serial.println();
            }
            _cmdsV[i].fn(String(valStr).substring(0, valLen));
            found = true; break;
          }
        }
      }

      if (found) {
        for (uint8_t w = 0; w < _watchdogCount; w++) {
          if (_watchdogList[w].key.length() == keyLen &&
              memcmp(_watchdogList[w].key.c_str(), keyStr, keyLen) == 0) {
            _watchdogList[w].lastMs = millis();
            _watchdogFired &= ~(1u << w);
            break;
          }
        }
      } else {
        Serial.print(F("Unbekannter Befehl: "));
        Serial.write(reinterpret_cast<const uint8_t*>(keyStr), keyLen);
        Serial.println();
      }
    } else {
      // Befehl ohne Wert
      bool found = false;
      for (uint8_t i = 0; i < _cmdCount; i++) {
        if (_cmds[i].key.length() == partLen &&
            memcmp(_cmds[i].key.c_str(), p, partLen) == 0) {
          if (debugMode & CCA_DEBUG_IN) {
            Serial.print(F("[CCA] IN  "));
            Serial.write(reinterpret_cast<const uint8_t*>(p), partLen);
            Serial.println(F(" (kein Wert)"));
          }
          _cmds[i].fn(); found = true; break;
        }
      }
      if (!found) {
        Serial.print(F("Unbekannter Befehl: "));
        Serial.write(reinterpret_cast<const uint8_t*>(p), partLen);
        Serial.println();
      }
    }

    if (!comma) break;
    p = comma + 1;
  }
}

void CCARemote::watchdog(String cmd, unsigned long timeoutMs) {
  for (uint8_t i = 0; i < _watchdogCount; i++) {
    if (_watchdogList[i].key == cmd) { _watchdogList[i].timeoutMs = timeoutMs; _watchdogList[i].lastMs = millis(); _watchdogFired &= ~(1u << i); return; }
  }
  if (_watchdogCount < CCA_MAX_RECEIVERS) {
    _watchdogList[_watchdogCount].key       = cmd;
    _watchdogList[_watchdogCount].timeoutMs = timeoutMs;
    _watchdogList[_watchdogCount].lastMs    = millis();
    _watchdogFired &= ~(1u << _watchdogCount);
    _watchdogCount++;
  }
}

void CCARemote::_checkWatchdogs() {
  unsigned long now = millis();
  for (uint8_t i = 0; i < _watchdogCount; i++) {
    if ((_watchdogFired >> i) & 1u) continue;
    if (now - _watchdogList[i].lastMs < _watchdogList[i].timeoutMs) continue;
    _watchdogFired |= (1u << i);
    const String& key = _watchdogList[i].key;
    for (uint8_t j = 0; j < _recvCount; j++) {
      if (_recv[j].key == key) {
        switch (_recv[j].type) {
          case _CCARecv::INT_T:    *((int*)   _recv[j].ptr) = 0;     break;
          case _CCARecv::BOOL_T:   *((bool*)  _recv[j].ptr) = false; break;
          case _CCARecv::FLOAT_T:  *((float*) _recv[j].ptr) = 0.0f;  break;
          case _CCARecv::STRING_T: *((String*)_recv[j].ptr) = "";    break;
        }
        if (debugMode & CCA_DEBUG_IN) { Serial.print(F("[CCA] WD  ")); Serial.println(key); }
        break;
      }
    }
    for (uint8_t j = 0; j < _colorRecvCount; j++) {
      if (_colorRecv[j].key == key) {
        *_colorRecv[j].r = 0; *_colorRecv[j].g = 0; *_colorRecv[j].b = 0;
        if (debugMode & CCA_DEBUG_IN) { Serial.print(F("[CCA] WD  ")); Serial.println(key); }
        break;
      }
    }
    for (uint8_t j = 0; j < _cmdVCount; j++) {
      if (_cmdsV[j].key == key) { _cmdsV[j].fn("0"); break; }
    }
    for (uint8_t j = 0; j < _cmdCount; j++) {
      if (_cmds[j].key == key) { _cmds[j].fn(); break; }
    }
  }
}

// ================================================================
//  Gemeinsame Methoden
// ================================================================

void CCARemote::_resyncDisplay() {
  for (uint8_t i = 0; i < _displayCount; i++) {
    sendInternal(_display[i].key, _display[i].value);
  }
}

void CCARemote::debug(CCADebugMode mode, unsigned long baudRate) {
  debugMode = mode;
  if (mode != CCA_DEBUG_OFF) {
    Serial.begin(baudRate);
#if defined(ESP8266)
    delay(3000);
#endif
  }
  if (mode == CCA_DEBUG_OFF)  Serial.println(F("[CCA] Debug-Modus deaktiviert"));
  if (mode == CCA_DEBUG_IN)   Serial.println(F("[CCA] Debug-Modus: nur IN"));
  if (mode == CCA_DEBUG_OUT)  Serial.println(F("[CCA] Debug-Modus: nur OUT"));
  if (mode == CCA_DEBUG_ALL)  Serial.println(F("[CCA] Debug-Modus: IN + OUT"));
}

void CCARemote::_sendIfChanged(String key, String value) {
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

void CCARemote::send(String key, bool value) {
  _sendIfChanged(key, value ? "1" : "0");
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
