// ============================================================================
// COASTAL DRIFFTER — XBEE SENDER (Upload ke Arduino BASE)
// ============================================================================
// Test komunikasi XBee dua arah: kirim & terima balasan dari Rover
//
// Wiring:
//   XBee TX → Arduino pin 2 (RX)
//   XBee RX → Arduino pin 8 (TX)
//   XBee VCC → 3.3V    (⚠️ jangan 5V!)
//   XBee GND → GND
//
// USB tetap dicolok ke PC — buka Serial Monitor @ 115200 untuk lihat output
// ============================================================================

#include <SoftwareSerial.h>

SoftwareSerial xbee(2, 8);   // RX=2, TX=8

unsigned long lastSend = 0;
unsigned long lastStatus = 0;
unsigned long sentCount = 0;
unsigned long recvCount = 0;
unsigned long lastRecvTime = 0;

// Packet uji — dikirim setiap 1 detik
// Format: "PING:<counter>,<millis>"
char packetBuf[40];

void sendPing() {
  sentCount++;
  snprintf(packetBuf, sizeof(packetBuf), "PING:%lu,%lu", sentCount, millis());

  xbee.print(packetBuf);
  Serial.print(F("[SEND] "));
  Serial.println(packetBuf);
}

void printStatus() {
  Serial.println();
  Serial.println(F("══════════════════════════════════"));
  Serial.print(F("  Tipe         : BASE (Sender)"));
  Serial.print(F("\n  Sent count   : "));
  Serial.print(sentCount);
  Serial.print(F("\n  Recv count   : "));
  Serial.print(recvCount);
  Serial.print(F("\n  Last recv    : "));
  if (lastRecvTime == 0) {
    Serial.print(F("Belum ada data masuk"));
  } else {
    unsigned long ago = (millis() - lastRecvTime) / 1000;
    Serial.print(ago);
    Serial.print(F(" detik lalu"));
  }
  Serial.println(F("\n══════════════════════════════════"));
  Serial.println();
}

void setup() {
  Serial.begin(115200);
  xbee.begin(9600);

  Serial.println();
  Serial.println(F("╔══════════════════════════════════════╗"));
  Serial.println(F("║    XBEE TEST — BASE (SENDER)         ║"));
  Serial.println(F("╠══════════════════════════════════════╣"));
  Serial.println(F("║  Kirim 'PING:N,millis' setiap 1 detik ║"));
  Serial.println(F("║  Terima balasan PONG dari Rover       ║"));
  Serial.println(F("╚══════════════════════════════════════╝"));
  Serial.println();
  Serial.println(F("Menunggu 3 detik sebelum mulai..."));
  delay(3000);
  Serial.println(F("▶ MULAI TRANSMISI"));
  Serial.println();
}

void loop() {
  // --- KIRIM PING tiap 1 detik ---
  if (millis() - lastSend >= 1000) {
    lastSend = millis();
    sendPing();
  }

  // --- TERIMA PONG / data dari Rover ---
  static char rxLine[64];
  static int rxIdx = 0;

  while (xbee.available()) {
    char c = xbee.read();
    Serial.write(c);   // echo mentah ke Serial Monitor

    if (c == '\n' || rxIdx >= (int)sizeof(rxLine) - 1) {
      rxLine[rxIdx] = '\0';
      if (rxIdx > 0) {
        recvCount++;
        lastRecvTime = millis();
        Serial.print(F("  ← [RECV #"));
        Serial.print(recvCount);
        Serial.print(F("] RX: "));
        Serial.println(rxLine);
      }
      rxIdx = 0;
    } else {
      rxLine[rxIdx++] = c;
    }
  }

  // --- STATUS REPORT tiap 5 detik ---
  if (millis() - lastStatus >= 5000) {
    lastStatus = millis();
    printStatus();
  }
}
