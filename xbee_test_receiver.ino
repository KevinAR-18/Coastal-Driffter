// ============================================================================
// COASTAL DRIFFTER — XBEE RECEIVER (Upload ke Arduino ROVER)
// ============================================================================
// Test komunikasi XBee dua arah: terima PING dr Base, kirim balik PONG + counter
//
// Wiring:
//   XBee TX → Arduino pin 2 (RX)
//   XBee RX → Arduino pin 8 (TX)
//   XBee VCC → 3.3V    (⚠️ jangan 5V!)
//   XBee GND → GND
//
// ⚠️ PENTING: Setelah upload, CABUT USB, lalu power Arduino pakai baterai.
//    Kalau mau lihat Serial Monitor Rover, pakai laptop kedua.
//
// Indikator:
//   LED built-in (pin 13) berkedip saat terima data dari Base
//   Kalau USB dicolok ke laptop, bisa buka Serial Monitor @ 115200
// ============================================================================

#include <SoftwareSerial.h>

SoftwareSerial xbee(2, 8);   // RX=2, TX=8

unsigned long recvCount = 0;
unsigned long sentCount = 0;
unsigned long lastStatus = 0;

void sendPong(const char* pingInfo) {
  sentCount++;
  xbee.print(F("PONG:"));
  xbee.print(recvCount);
  xbee.print(F(",rssi="));
  xbee.print(sentCount);   // pseudo-rssi = how many pongs sent
  xbee.print(F(",millis="));
  xbee.print(millis());
  xbee.println();
}

void blinkLed(int times, int msOn) {
  for (int i = 0; i < times; i++) {
    digitalWrite(LED_BUILTIN, HIGH);
    delay(msOn);
    digitalWrite(LED_BUILTIN, LOW);
    if (i < times - 1) delay(msOn);
  }
}

void setup() {
  xbee.begin(9600);
  pinMode(LED_BUILTIN, OUTPUT);

  // LED boot signal: 3x kedip cepat = program jalan
  blinkLed(3, 80);
  delay(500);

  // Kalau ada USB (debug mode), print info
  if (Serial) {
    Serial.begin(115200);
    Serial.println();
    Serial.println(F("╔══════════════════════════════════════╗"));
    Serial.println(F("║   XBEE TEST — ROVER (RECEIVER)       ║"));
    Serial.println(F("╠══════════════════════════════════════╣"));
    Serial.println(F("║  Terima PING dr Base, balas PONG     ║"));
    Serial.println(F("║  LED berkedip = ada data masuk       ║"));
    Serial.println(F("╚══════════════════════════════════════╝"));
    Serial.println(F("Menunggu data dari Base..."));
    Serial.println();
  }
}

void loop() {
  // --- TERIMA PING dari Base ---
  static char rxLine[64];
  static int rxIdx = 0;

  while (xbee.available()) {
    char c = xbee.read();

    if (c == '\n' || rxIdx >= (int)sizeof(rxLine) - 1) {
      rxLine[rxIdx] = '\0';
      if (rxIdx > 0) {
        recvCount++;

        // Cetak ke USB kalau tersambung (debug mode)
        if (Serial) {
          Serial.print(F("[RECV #"));
          Serial.print(recvCount);
          Serial.print(F("] "));
          Serial.println(rxLine);
        }

        // Balas PONG ke Base
        sendPong(rxLine);

        // LED kedip 1x
        digitalWrite(LED_BUILTIN, HIGH);
        delay(50);
        digitalWrite(LED_BUILTIN, LOW);
      }
      rxIdx = 0;
    } else {
      rxLine[rxIdx++] = c;
    }
  }

  // --- STATUS REPORT tiap 5 detik (hanya kalau USB tersambung) ---
  if (Serial && (millis() - lastStatus >= 5000)) {
    lastStatus = millis();
    Serial.println();
    Serial.print(F("[STATUS] Received: "));
    Serial.print(recvCount);
    Serial.print(F(" | Sent: "));
    Serial.println(sentCount);
  }
}
