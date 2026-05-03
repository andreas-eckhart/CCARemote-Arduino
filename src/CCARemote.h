/*
 * CCARemote.h – Abstract Base Class Declaration
 *
 * Based on the diploma thesis by L. Eder and E. Duyar (HTL Anichstraße)
 * Extended by A. Eckhart with kind permission of the original authors.
 *
 * Version: 1.0.0 | 2026-05-03 | MIT – see LICENSE
 */

#ifndef CCAREMOTE_H
#define CCAREMOTE_H

#include <Arduino.h>
#include <functional>
#include <map>
#include <vector>

// Abstrakte Basisklasse - nicht direkt verwenden!
// Verwende: CCARemoteBLE, CCARemoteWiFi
class CCARemote {
  public:
    CCARemote(String name, String prefix = "CCA-");
    virtual ~CCARemote() {}

    virtual void handle()      = 0;
    virtual bool isConnected() = 0;

    void onCommand(String cmd, std::function<void()> callback);
    void onCommand(String cmd, std::function<void(String)> callback);

    // Variable Binding: remote.handle() aktualisiert die Variable automatisch
    void receive(String cmd, int&    var);   // fuer Slider (0-255), Zahlenwerte
    void receive(String cmd, bool&   var);   // fuer Button, Switch (true/false)
    void receive(String cmd, float&  var);   // fuer Dezimalwerte
    void receive(String cmd, String& var);   // fuer Texteingabe

    // Debug-Modus: empfangene und gesendete Werte im Seriellen Monitor ausgeben
    void debug(bool enable = true, unsigned long baudRate = 9600);

    void send(String message);
    void send(String key, String value);
    void send(String key, int value);
    void send(String key, float value);
    void send(String key, float value, int decimals);

  protected:
    String deviceName;
    std::map<String, std::function<void()>>       commands;
    std::map<String, std::function<void(String)>> commandsWithValue;
    std::map<String, String> displayValues;
    String lastCommand;
    bool   commandReceived;
    bool   debugEnabled;

    void processCommand(String cmd);
    virtual void sendInternal(String key, String value) = 0;
};

#endif // CCAREMOTE_H
