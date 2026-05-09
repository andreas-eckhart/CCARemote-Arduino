/*
 * CCARemote.h – Abstract Base Class Declaration
 *
 * Platform detection (automatic):
 *   AVR (Uno/Nano) → fixed arrays + function pointers (no std::function/map)
 *   ESP32 / other  → std::function + std::map (full C++ standard library)
 *
 * Based on the diploma thesis by L. Eder and E. Duyar (HTL Anichstraße)
 * Extended by A. Eckhart with kind permission of the original authors.
 *
 * MIT – see LICENSE
 */

#ifndef CCAREMOTE_H
#define CCAREMOTE_H

// Version der Bibliothek
#define CCAREMOTE_VERSION "1.0.0"

#include <Arduino.h>

// Debug-Modus Flags
enum CCADebugMode {
  CCA_DEBUG_OFF = 0,
  CCA_DEBUG_IN  = 1,
  CCA_DEBUG_OUT = 2,
  CCA_DEBUG_ALL = 3
};

// ================================================================
#if defined(__AVR__)
// ================================================================
//  AVR (Uno/Nano) – kein std::function, keine std::map
//  Maximale Anzahl registrierbarer Callbacks / Variablen
// ================================================================
#ifndef CCA_MAX_CALLBACKS
  #define CCA_MAX_CALLBACKS 8
#endif
#ifndef CCA_MAX_RECEIVERS
  #define CCA_MAX_RECEIVERS 8
#endif
#ifndef CCA_MAX_DISPLAY
  #define CCA_MAX_DISPLAY 8
#endif

struct _CCACmd  { String key; void (*fn)(); };
struct _CCACmdV { String key; void (*fn)(String); };

struct _CCARecv {
  String key;
  enum Type : uint8_t { INT_T, BOOL_T, FLOAT_T, STRING_T } type;
  void* ptr;
};

struct _CCADisplay { String key; String value; };

// ================================================================
#else
// ================================================================
//  ESP32 / andere – volle C++ Standardbibliothek
// ================================================================
#include <functional>
#include <map>

#endif // __AVR__

// ================================================================
//  Gemeinsame Basisklasse
// ================================================================
class CCARemote {
  public:
    CCARemote(String name, String prefix = "CCA-");
    virtual ~CCARemote() {}

    virtual void handle()      = 0;
    virtual bool isConnected() = 0;

#if defined(__AVR__)
    void onCommand(String cmd, void (*callback)());
    void onCommand(String cmd, void (*callback)(String));
#else
    void onCommand(String cmd, std::function<void()> callback);
    void onCommand(String cmd, std::function<void(String)> callback);
#endif

    void receive(String cmd, int&    var);
    void receive(String cmd, bool&   var);
    void receive(String cmd, float&  var);
    void receive(String cmd, String& var);

    void debug(CCADebugMode mode = CCA_DEBUG_ALL, unsigned long baudRate = 9600);

    void send(String message);
    void send(String key, String value);
    void send(String key, int value);
    void send(String key, float value);
    void send(String key, float value, int decimals);

  protected:
    String       deviceName;
    String       lastCommand;
    bool         commandReceived;
    CCADebugMode debugMode;

    void processCommand(String cmd);
    void _resyncDisplay();
    void _sendIfChanged(String key, String value);
    bool _pendingResync;
    virtual void sendInternal(String key, String value) = 0;

    // Display-Werte: auf AVR fixes Array, sonst std::map
#if defined(__AVR__)
    _CCADisplay _display[CCA_MAX_DISPLAY];
    uint8_t     _displayCount;
#else
    std::map<String, String> displayValues;
#endif

  private:
#if defined(__AVR__)
    _CCACmd  _cmds[CCA_MAX_CALLBACKS];
    _CCACmdV _cmdsV[CCA_MAX_CALLBACKS];
    uint8_t  _cmdCount;
    uint8_t  _cmdVCount;
    _CCARecv _recv[CCA_MAX_RECEIVERS];
    uint8_t  _recvCount;
#else
    std::map<String, std::function<void()>>       commands;
    std::map<String, std::function<void(String)>> commandsWithValue;
#endif
};

#endif // CCAREMOTE_H
