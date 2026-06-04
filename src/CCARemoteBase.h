/*
 * CCARemoteBase.h – Abstract Base Class Declaration
 *
 * Platform detection (automatic):
 *   AVR (Uno/Nano) → fixed arrays + function pointers (no std::function/map)
 *   ESP32 / other  → same flat-array approach, larger default limits
 *
 * Based on the diploma thesis by L. Eder and E. Duyar (HTL Anichstraße)
 * Extended by A. Eckhart with kind permission of the original authors.
 *
 * MIT – see LICENSE
 */

#ifndef CCAREMOTE_BASE_H
#define CCAREMOTE_BASE_H

// Version der Bibliothek
#define CCA_LIB_VERSION      "1.2.1"
// Protokollversion – wird nur bei Breaking Changes erhöht
#define CCA_PROTOCOL_VERSION "2"
#define CCA_PLATFORM         "arduino"

#include <Arduino.h>

// ================================================================
//  Persistente Zustandsspeicherung (mit #define CCA_NO_PERSIST deaktivierbar)
// ================================================================
#ifndef CCA_NO_PERSIST
  #if defined(ESP32)
    #include <Preferences.h>
  #elif defined(ESP8266) || defined(__AVR__)
    #include <EEPROM.h>
  #endif
#endif

// Debug-Modus Flags
enum CCADebugMode {
  CCA_DEBUG_OFF = 0,
  CCA_DEBUG_IN  = 1,
  CCA_DEBUG_OUT = 2,
  CCA_DEBUG_ALL = 3
};

// ================================================================
//  Maximale Array-Größen
//  AVR: klein (RAM-Mangel), ESP32/andere: größer
// ================================================================
#if defined(__AVR__)
  #ifndef CCA_MAX_CALLBACKS
    #define CCA_MAX_CALLBACKS 8
  #endif
  #ifndef CCA_MAX_RECEIVERS
    #define CCA_MAX_RECEIVERS 8
  #endif
  #ifndef CCA_MAX_DISPLAY
    #define CCA_MAX_DISPLAY 8
  #endif
  #ifndef CCA_MAX_COLOR
    #define CCA_MAX_COLOR 4
  #endif
#else
  #ifndef CCA_MAX_CALLBACKS
    #define CCA_MAX_CALLBACKS 16
  #endif
  #ifndef CCA_MAX_RECEIVERS
    #define CCA_MAX_RECEIVERS 16
  #endif
  #ifndef CCA_MAX_DISPLAY
    #define CCA_MAX_DISPLAY 16
  #endif
  #ifndef CCA_MAX_COLOR
    #define CCA_MAX_COLOR 8
  #endif
#endif

// ================================================================
//  EEPROM-Konstanten (AVR + ESP8266, nach MAX-Defines)
// ================================================================
#ifndef CCA_NO_PERSIST
  #if defined(ESP8266) || defined(__AVR__)
    #define CCA_EEPROM_MAGIC     0xCA
    #define CCA_EEPROM_BASE      0
    #define CCA_RECV_SLOT_SZ     5                // type(1) + value(4)
    #define CCA_COLOR_SLOT_SZ    (3 * sizeof(int))
    #define CCA_EEPROM_RECV_OFF  (CCA_EEPROM_BASE + 1)
    #define CCA_EEPROM_COLOR_OFF (CCA_EEPROM_RECV_OFF + CCA_MAX_RECEIVERS * CCA_RECV_SLOT_SZ)
    #define CCA_EEPROM_SIZE      (CCA_EEPROM_COLOR_OFF + CCA_MAX_COLOR * CCA_COLOR_SLOT_SZ)
  #endif
#endif

struct _CCACmd  { String key; void (*fn)(); };
struct _CCACmdV { String key; void (*fn)(String); };

struct _CCARecv {
  String key;
  enum Type : uint8_t { INT_T, BOOL_T, FLOAT_T, STRING_T } type;
  void* ptr;
  bool resync;
};

struct _CCAColorRecv { String key; int* r; int* g; int* b; bool resync; };
struct _CCADisplay   { String key; String value; };
struct _CCAWatchdog  { String key; unsigned long timeoutMs; unsigned long lastMs; };

// ================================================================
//  Gemeinsame Basisklasse
// ================================================================
class CCARemote {
  public:
    CCARemote(String name, String prefix = "CCA-",
              CCADebugMode  debugLevel = CCA_DEBUG_OFF,
              unsigned long baudRate   = 115200);
    virtual ~CCARemote() {}

    virtual void handle()      = 0;
    virtual bool isConnected() = 0;

    void onCommand(String cmd, void (*callback)());
    void onCommand(String cmd, void (*callback)(String));

    void receive(String cmd, int&    var, bool resync = false);
    void receive(String cmd, bool&   var, bool resync = false);
    void receive(String cmd, float&  var, bool resync = false);
    void receive(String cmd, String& var, bool resync = false);
    void receiveColor(String cmd, int& r, int& g, int& b, bool resync = false);
    void watchdog(String cmd, unsigned long timeoutMs);

    void debug(CCADebugMode mode = CCA_DEBUG_ALL, unsigned long baudRate = 9600);
#ifndef CCA_NO_PERSIST
    void clearState();
#endif

    // AVR: pointer only – no RAM copy. ESP32/non-AVR: copy into _display as usual.
#if defined(__AVR__)
    void setProfile(const char* configString) {
      _profileConfigPtr = configString;
      _profileIsPgm     = false;
    }
    void setProfile(const __FlashStringHelper* configString) {
      _profileConfigPtr = (const char*)configString;
      _profileIsPgm     = true;
    }
#else
    void setProfile(const char* configString) {
      if (_displayCount < CCA_MAX_DISPLAY)
        _display[_displayCount++] = { "profileConfig", String(configString) };
    }
    void setProfile(const __FlashStringHelper* configString) {
      if (_displayCount < CCA_MAX_DISPLAY)
        _display[_displayCount++] = { "profileConfig", String(configString) };
    }
#endif

    void send(String message);
    void send(String key, String value);
    void send(String key, const char* value);
    void send(String key, bool value);
    void send(String key, int value);
    void send(String key, float value);
    void send(String key, double value);
    void send(String key, float value, int decimals);

    void sendAlways(String key, String value);
    void sendAlways(String key, int value);
    void sendAlways(String key, float value);
    void sendAlways(String key, double value);
    void sendAlways(String key, float value, int decimals);

  protected:
    String        deviceName;
    String        lastCommand;
    bool          commandReceived;
    CCADebugMode  debugMode;
    unsigned long _serialBaudRate;

    void processCommand(const char* cmd);
    void _resyncDisplay();
#ifndef CCA_NO_PERSIST
    void _loadState();
    void _saveState();
#endif
    void _sendIfChanged(String key, String value);
    void _sendAlways(String key, String value);
    void _checkWatchdogs();
    bool _pendingResync;
    bool _stateLoaded;
    virtual void sendInternal(String key, String value) = 0;
#if defined(__AVR__)
    // Streams profileConfig from PROGMEM/RAM without heap allocation.
    // Overridden by platform-specific subclass (BLE/WiFi).
    virtual void _sendProfileConfig() {}
    const char* _profileConfigPtr;
    bool        _profileIsPgm;
#endif

    _CCADisplay _display[CCA_MAX_DISPLAY];
    uint8_t     _displayCount;

  private:
#ifndef CCA_NO_PERSIST
  #if defined(ESP32)
    Preferences   _prefs;
  #endif
#endif
    _CCACmd       _cmds[CCA_MAX_CALLBACKS];
    _CCACmdV      _cmdsV[CCA_MAX_CALLBACKS];
    uint8_t       _cmdCount;
    uint8_t       _cmdVCount;
    _CCARecv      _recv[CCA_MAX_RECEIVERS];
    uint8_t       _recvCount;
    _CCAColorRecv _colorRecv[CCA_MAX_COLOR];
    uint8_t       _colorRecvCount;
    _CCAWatchdog  _watchdogList[CCA_MAX_RECEIVERS];
    uint8_t       _watchdogCount;
    uint16_t      _watchdogFired;  // bit i = watchdog i has fired, re-armed on real data
};

#endif // CCAREMOTE_BASE_H
