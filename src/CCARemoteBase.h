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
#define CCA_LIB_VERSION      "1.2.0"
// Protokollversion – wird nur bei Breaking Changes erhöht
#define CCA_PROTOCOL_VERSION "2"
#define CCA_PLATFORM         "arduino"

#include <Arduino.h>

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

struct _CCACmd  { String key; void (*fn)(); };
struct _CCACmdV { String key; void (*fn)(String); };

struct _CCARecv {
  String key;
  enum Type : uint8_t { INT_T, BOOL_T, FLOAT_T, STRING_T } type;
  void* ptr;
};

struct _CCAColorRecv { String key; int* r; int* g; int* b; };
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

    void receive(String cmd, int&    var);
    void receive(String cmd, bool&   var);
    void receive(String cmd, float&  var);
    void receive(String cmd, String& var);
    void receiveColor(String cmd, int& r, int& g, int& b);
    void watchdog(String cmd, unsigned long timeoutMs);

    void debug(CCADebugMode mode = CCA_DEBUG_ALL, unsigned long baudRate = 9600);

    void setProfile(const char* configString) {
      if (_displayCount < CCA_MAX_DISPLAY)
        _display[_displayCount++] = { "profileConfig", String(configString) };
    }
    void setProfile(const __FlashStringHelper* configString) {
      if (_displayCount < CCA_MAX_DISPLAY)
        _display[_displayCount++] = { "profileConfig", String(configString) };
    }

    void send(String message);
    void send(String key, String value);
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
    void _sendIfChanged(String key, String value);
    void _sendAlways(String key, String value);
    void _checkWatchdogs();
    bool _pendingResync;
    virtual void sendInternal(String key, String value) = 0;

    _CCADisplay _display[CCA_MAX_DISPLAY];
    uint8_t     _displayCount;

  private:
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
