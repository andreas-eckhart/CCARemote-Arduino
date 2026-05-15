/*
 * CCARemote.h – Umbrella-Header
 *
 * Bindet automatisch das richtige Transportprotokoll ein.
 * Muss NACH den Konfigurations-#defines eingebunden werden!
 *
 * Verwendung im Sketch:
 *
 *   #define DEVICE_NAME  "MeinName"    // Gerätename
 *   #define CONNECTION   CCA_BLE       // CCA_BLE oder CCA_WIFI
 *   #define PASSWORD     ""            // Passwort (leer = ohne)
 *   #define DEBUG_LEVEL  CCA_DEBUG_ALL // Debugging
 *   #include <CCARemote.h>
 *
 * Developed by A. Eckhart (HTL Anichstraße) - MIT – see LICENSE
 */

#pragma once

#define CCA_BLE  1
#define CCA_WIFI 2

// Standardwerte für optionale Defines
#ifndef DEVICE_PREFIX
  #define DEVICE_PREFIX "CCA-"
#endif
#ifndef TCP_PORT
  #define TCP_PORT 4210
#endif
#ifndef DEBUG_LEVEL
  #define DEBUG_LEVEL CCA_DEBUG_OFF
#endif
#ifndef PASSWORD
  #define PASSWORD ""
#endif
#ifndef BAUD_RATE
  #define BAUD_RATE 115200
#endif

// Pflicht-Defines prüfen
#if !defined(DEVICE_NAME)
  #error "CCARemote: DEVICE_NAME nicht definiert! Beispiel: #define DEVICE_NAME \"MeinName\""
#endif
#if !defined(CONNECTION)
  #error "CCARemote: CONNECTION nicht definiert! Beispiel: #define CONNECTION CCA_BLE"
#endif
#if (CONNECTION != CCA_BLE) && (CONNECTION != CCA_WIFI)
  #error "CCARemote: Ungueltige CONNECTION – nur CCA_BLE oder CCA_WIFI erlaubt!"
#endif

// Transport einbinden und remote-Objekt anlegen
#if (CONNECTION == CCA_WIFI)
  #include "CCARemoteWiFi.h"
  CCARemoteWiFi remote(DEVICE_NAME, DEVICE_PREFIX, PASSWORD, TCP_PORT, DEBUG_LEVEL, BAUD_RATE);
#else
  #include "CCARemoteBLE.h"
  CCARemoteBLE  remote(DEVICE_NAME, DEVICE_PREFIX, PASSWORD, DEBUG_LEVEL, BAUD_RATE);
#endif
