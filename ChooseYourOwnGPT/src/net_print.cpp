#include "net_print.h"
#include <WiFi.h>

static WiFiServer printServer(NET_PRINT_PORT);
static Adafruit_Thermal *printerPtr = nullptr;
static Stream *printerSerialPtr = nullptr;
static bool listening = false;

void netPrintSetup(Adafruit_Thermal &printer, Stream &printerSerial) {
  printerPtr = &printer;
  printerSerialPtr = &printerSerial;
}

void netPrintBegin() {
  if (listening) printServer.end();
  printServer.begin();
  printServer.setNoDelay(true);
  listening = true;
  Serial.printf("Network printing on port %u\n", NET_PRINT_PORT);
}

void netPrintLoop() {
  if (!listening || !printerPtr || !printerSerialPtr) return;

  WiFiClient client = printServer.accept();
  if (!client) return;

  Serial.printf("Print job from %s\n", client.remoteIP().toString().c_str());

  uint8_t buf[256];
  size_t total = 0;
  unsigned long lastData = millis();

  while (client.connected() || client.available()) {
    int n = client.read(buf, sizeof(buf));
    if (n > 0) {
      // Write bytes directly rather than through printer.write(), which
      // strips 0x0D and would corrupt binary data such as bitmaps.  The
      // DTR handshake in timeoutWait() keeps the printer buffer from
      // overflowing; TCP flow control throttles the sender in turn.
      for (int i = 0; i < n; i++) {
        printerPtr->timeoutWait();
        printerSerialPtr->write(buf[i]);
      }
      total += n;
      lastData = millis();
    } else if (millis() - lastData > NET_PRINT_IDLE_MS) {
      Serial.println("Print job idle timeout");
      break;
    } else {
      delay(1);
    }
  }
  client.stop();

  // Restore the formatting the story printer expects, in case the job
  // changed size, justification, inverse, etc.
  printerPtr->setDefault();

  Serial.printf("Print job done (%u bytes)\n", (unsigned)total);
}
