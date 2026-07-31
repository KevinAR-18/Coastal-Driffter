#include <Arduino.h>

// Definisi Serial Port:
// Serial1: Terhubung ke simpleRTK2B Rover (RX1=19, TX1=18 di Arduino Mega / GPIO 16, 17 di ESP32)
// Serial2: Terhubung ke Radio Telemetry / LoRa Transceiver (RX2=17, TX2=16 di Arduino Mega / GPIO 4, 2 di ESP32)

#define RTK_SERIAL Serial1
#define RADIO_SERIAL Serial2

#pragma pack(push, 1)
struct TelemetryPacket {
  uint8_t  header1 = 0xAA;
  uint8_t  header2 = 0xBB;
  uint8_t  rover_id = 1;
  int32_t  lat;        // Deg * 1e7
  int32_t  lon;        // Deg * 1e7
  int32_t  alt_mm;     // mm
  uint8_t  fix_status; // 4=RTK FIX, 5=RTK FLOAT, 1=Single
  uint16_t h_acc_mm;   // mm
  uint16_t speed_01kmh;// 0.1 km/h
  uint16_t heading_01deg; // 0.1 deg
  int16_t  rel_N_cm;   // cm
  int16_t  rel_E_cm;   // cm
  uint8_t  checksum;
};
#pragma pack(pop)

TelemetryPacket telePacket;
unsigned long lastSendTime = 0;

uint8_t calculateChecksum(uint8_t *data, size_t len) {
  uint8_t crc = 0;
  for (size_t i = 0; i < len; i++) crc ^= data[i];
  return crc;
}

void setup() {
  Serial.begin(115200);      // Debug Serial PC
  RTK_SERIAL.begin(38400);   // Baudrate ZED-F9P Rover
  RADIO_SERIAL.begin(57600); // Baudrate Radio Telemetry

  Serial.println(F("[ROVER] System Initialized. Waiting for RTCM3 & UBX Data..."));
}

void loop() {
  // 1. Forward data koreksi RTCM3 dari Radio Telemetry ke simpleRTK2B Rover
  while (RADIO_SERIAL.available()) {
    uint8_t b = RADIO_SERIAL.read();
    RTK_SERIAL.write(b); // Feed RTCM3 correction to ZED-F9P
  }

  // 2. Transmit Telemetry Packet dari Rover ke Base (5 Hz = setiap 200ms)
  if (millis() - lastSendTime >= 200) {
    lastSendTime = millis();

    // Data hasil parse dari ZED-F9P (Contoh nilai bergerak)
    telePacket.lat = -61753924;      // -6.1753924 Deg
    telePacket.lon = 1068271532;     // 106.8271532 Deg
    telePacket.alt_mm = 15420;       // 15.42 m
    telePacket.fix_status = 4;       // 4 = RTK FIX (1-2 cm)
    telePacket.h_acc_mm = 12;        // 12 mm = 1.2 cm
    telePacket.speed_01kmh = 145;    // 14.5 km/h
    telePacket.heading_01deg = 1284; // 128.4 deg
    telePacket.rel_N_cm = 4215;      // 42.15 m
    telePacket.rel_E_cm = 1870;      // 18.70 m

    // Hitung Checksum
    telePacket.checksum = calculateChecksum((uint8_t*)&telePacket, sizeof(TelemetryPacket) - 1);

    // Kirim paket telemetri via Radio Telemetry ke Base
    RADIO_SERIAL.write((uint8_t*)&telePacket, sizeof(TelemetryPacket));
  }
}
