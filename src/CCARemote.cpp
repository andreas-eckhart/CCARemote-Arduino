/*
 * CCARemote.cpp – Abstract Base Class Implementation
 *
 * Based on the diploma thesis by L. Eder and E. Duyar (HTL Anichstraße)
 * Extended by A. Eckhart with kind permission of the original authors.
 *
 * Version: 1.1.0 | 2026-05-07 | MIT – see LICENSE
 */

#include "CCARemote.h"

// ================================================================
#if defined(__AVR__)
// ================================================================
//  AVR (Uno/Nano)
// ================================================================

CCARemote::CCARemote(String name, String prefix) {
  deviceName      = prefix + name;
  commandReceived = false;
  lastCommand     = "";
  debugMode       = CCA_DEBUG_OFF;
  _cmdCount       = 0;
  _cmdVCount      = 0;
  _recvCount      = 0;
  _displayCount   = 0;
  _pendingResync  = false;
}

void CCARemote::onCommand(String cmd, void (*callback)()) {
  if (_cmdCount < CCA_MAX_CALLBACKS) {
    _cmds[_cmdCount++] = { cmd, callback };
    Serial.println("Befehl registriert: " + cmd);
  }
}

void CCARemote::onCommand(String cmd, void (*callback)(String)) {
  if (_cmdVCount < CCA_MAX_CALLBACKS) {
    _cmdsV[_cmdVCount++] = { cmd, callback };
    Serial.println("Befehl registriert: " + cmd + " (mit Wert)");
  }
}

void CCARemote::receive(String cmd, int& var) {
  if (_recvCount < CCA_MAX_RECEIVERS) {
    _recv[_recvCount++] = { cmd, _CCARecv::INT_T, &var };
    Serial.println("Variable gebunden: " + cmd + " (int)");
  }
}

void CCARemote::receive(String cmd, bool& var) {
  if (_recvCount < CCA_MAX_RECEIVERS) {
    _recv[_recvCount++] = { cmd, _CCARecv::BOOL_T, &var };
    Serial.println("Variable gebunden: " + cmd + " (bool)");
  }
}

void CCARemote::receive(String cmd, float& var) {
  if (_recvCount < CCA_MAX_RECEIVERS) {
    _recv[_recvCount++] = { cmd, _CCARecv::FLOAT_T, &var };
    Serial.println("Variable gebunden: " + cmd + " (float)");
  }
}

void CCARemote::receive(String cmd, String& var) {
  if (_recvCount < CCA_MAX_RECEIVERS) {
    _recv[_recvCount++] = { cmd, _CCARecv::STRING_T, &var };
    Serial.println("Variable gebunden: " + cmd + " (String)");
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

        // Variable-Bindings prüfen
        bool found = false;
        for (uint8_t i = 0; i < _recvCount; i++) {
          if (_recv[i].key == key) {
            if (debugMode & CCA_DEBUG_IN)
              Serial.println("[CCA] IN  " + key + " = " + value);
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
        // Callback mit Wert prüfen
        if (!found) {
          for (uint8_t i = 0; i < _cmdVCount; i++) {
            if (_cmdsV[i].key == key) {
              if (debugMode & CCA_DEBUG_IN)
                Serial.println("[CCA] IN  " + key + " = " + value);
              _cmdsV[i].fn(value);
              found = true;
              break;
            }
          }
        }
        if (!found) Serial.println("Unbekannter Befehl: " + key);
      } else {
        bool found = false;
        for (uint8_t i = 0; i < _cmdCount; i++) {
          if (_cmds[i].key == part) {
            if (debugMode & CCA_DEBUG_IN)
              Serial.println("[CCA] IN  " + part + " (kein Wert)");
            _cmds[i].fn();
            found = true;
            break;
          }
        }
        if (!found) Serial.println("Unbekannter Befehl: " + part);
      }
    }

    if (commaPos < 0) break;
    start = commaPos + 1;
  }
}

// ================================================================
#else
// ================================================================
//  ESP32 / andere – std::function + std::map
// ================================================================

CCARemote::CCARemote(String name, String prefix) {
  deviceName      = prefix + name;
  commandReceived = false;
  lastCommand     = "";
  debugMode       = CCA_DEBUG_OFF;
  _pendingResync  = false;
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
        } else {
          Serial.println("Unbekannter Befehl: " + command);
        }
      } else {
        if (commands.count(part) > 0) {
          if (debugMode & CCA_DEBUG_IN) Serial.println("[CCA] IN  " + part + " (kein Wert)");
          commands[part]();
        } else {
          Serial.println("Unbekannter Befehl: " + part);
        }
      }
    }

    if (commaPos < 0) break;
    start = commaPos + 1;
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
  if (mode != CCA_DEBUG_OFF) Serial.begin(baudRate);
  if (mode == CCA_DEBUG_OFF)  Serial.println("[CCA] Debug-Modus deaktiviert");
  if (mode == CCA_DEBUG_IN)   Serial.println("[CCA] Debug-Modus: nur IN");
  if (mode == CCA_DEBUG_OUT)  Serial.println("[CCA] Debug-Modus: nur OUT");
  if (mode == CCA_DEBUG_ALL)  Serial.println("[CCA] Debug-Modus: IN + OUT");
}

void CCARemote::send(String message) {
  int colonPos = message.indexOf(':');
  if (colonPos > 0) {
    String key   = message.substring(0, colonPos);
    String value = message.substring(colonPos + 1);
    if (debugMode & CCA_DEBUG_OUT) Serial.println("[CCA] OUT " + key + " = " + value);
    sendInternal(key, value);
  } else {
    if (debugMode & CCA_DEBUG_OUT) Serial.println("[CCA] OUT " + message);
    sendInternal(message, "");
  }
}

void CCARemote::send(String key, String value) {
  if (debugMode & CCA_DEBUG_OUT) Serial.println("[CCA] OUT " + key + " = " + value);
  sendInternal(key, value);
}

void CCARemote::send(String key, int value) {
  if (debugMode & CCA_DEBUG_OUT) Serial.println("[CCA] OUT " + key + " = " + String(value));
  sendInternal(key, String(value));
}

void CCARemote::send(String key, float value) {
  if (debugMode & CCA_DEBUG_OUT) Serial.println("[CCA] OUT " + key + " = " + String(value, 1));
  sendInternal(key, String(value, 1));
}

void CCARemote::send(String key, float value, int decimals) {
  if (debugMode & CCA_DEBUG_OUT) Serial.println("[CCA] OUT " + key + " = " + String(value, decimals));
  sendInternal(key, String(value, decimals));
}
