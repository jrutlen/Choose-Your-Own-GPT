#ifndef NET_PRINT_H
#define NET_PRINT_H

#include <Arduino.h>
#include "Adafruit_Thermal.h"

// Raw network printing ("port 9100" style).
//
// A client opens a TCP connection, streams bytes and closes the connection.
// Every byte is forwarded unmodified to the thermal printer, so clients can
// send plain text as well as printer commands (bold, sizes, bitmaps, ...).
// A job ends when the client disconnects or goes idle for NET_PRINT_IDLE_MS.
static const uint16_t NET_PRINT_PORT = 9100;
static const unsigned long NET_PRINT_IDLE_MS = 3000;

// Store references to the printer and its serial port. Call once in setup().
void netPrintSetup(Adafruit_Thermal &printer, Stream &printerSerial);

// Start listening. Safe to call repeatedly (e.g. on every WiFi reconnect).
void netPrintBegin();

// Service a pending print job, if any. Blocks until the job finishes.
void netPrintLoop();

#endif // NET_PRINT_H
