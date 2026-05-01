#include "CCARemote.h"

CCARemote::CCARemote(String name, String prefix) {
  deviceName      = prefix + name;
  commandReceived = false;
  lastCommand     = "";
  debugEnabled    = false;
}

void CCARemote::onCommand(String cmd, std::function<void()> callback) {
  commands[cmd] = callback;
  Serial.println("Befehl registriert: " + cmd);
}

void CCARemote::onCommand(String cmd, std::function<void(String)> callback) {
  commandsWithValue[cmd] = callback;
  Serial.println("Befehl registriert: " + cmd + " (mit Wert)");
}

void CCARemote::debug(bool enable, unsigned long baudRate) {
  debugEnabled = enable;
  if (enable) {
    Serial.begin(baudRate);
  }
  Serial.println(enable ? "[CCA] Debug-Modus aktiviert" : "[CCA] Debug-Modus deaktiviert");
}

void CCARemote::receive(String cmd, int& var) {
  commandsWithValue[cmd] = [this, cmd, &var](String value) {
    var = value.toInt();
    if (debugEnabled) Serial.println("[CCA] IN  " + cmd + " = " + value);
  };
  Serial.println("Variable gebunden: " + cmd + " (int)");
}

void CCARemote::receive(String cmd, bool& var) {
  commandsWithValue[cmd] = [this, cmd, &var](String value) {
    var = (value == "1" || value == "true" || value == "on");
    if (debugEnabled) Serial.println("[CCA] IN  " + cmd + " = " + value);
  };
  Serial.println("Variable gebunden: " + cmd + " (bool)");
}

void CCARemote::receive(String cmd, float& var) {
  commandsWithValue[cmd] = [this, cmd, &var](String value) {
    var = value.toFloat();
    if (debugEnabled) Serial.println("[CCA] IN  " + cmd + " = " + value);
  };
  Serial.println("Variable gebunden: " + cmd + " (float)");
}

void CCARemote::receive(String cmd, String& var) {
  commandsWithValue[cmd] = [this, cmd, &var](String value) {
    var = value;
    if (debugEnabled) Serial.println("[CCA] IN  " + cmd + " = " + value);
  };
  Serial.println("Variable gebunden: " + cmd + " (String)");
}

void CCARemote::processCommand(String cmd) {
  // Kommaseparierte Pakete aufteilen (z.B. "varX:100,varY:-50")
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
          if (debugEnabled) Serial.println("[CCA] IN  " + part + " (kein Wert)");
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

void CCARemote::send(String message) {
  int colonPos = message.indexOf(':');
  if (colonPos > 0) {
    sendInternal(message.substring(0, colonPos), message.substring(colonPos + 1));
  } else {
    sendInternal(message, "");
  }
}

void CCARemote::send(String key, String value) {
  if (debugEnabled) Serial.println("[CCA] OUT " + key + " = " + value);
  sendInternal(key, value);
}

void CCARemote::send(String key, int value) {
  sendInternal(key, String(value));
}

void CCARemote::send(String key, float value) {
  sendInternal(key, String(value, 1));
}

void CCARemote::send(String key, float value, int decimals) {
  sendInternal(key, String(value, decimals));
}