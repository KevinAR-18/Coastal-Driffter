#include <Arduino.h>

// ============================================================================
// COASTAL DRIFFTER - BASE STATION FIRMWARE (RTCM3 Downlink + JSON Stream)
// ============================================================================
// Serial1: Terhubung ke simpleRTK2B Base ZED-F9P (RTCM3 Output)
// Serial2: Terhubung ke Radio Telemetry / LoRa Transceiver (RX2=17, TX2=16)
// Serial:  USB Serial ke Laptop / PC Dashboard (Format JSON Stream @ 115200 bps)
// ============================================================================

#define RTK_BASE_SERIAL Serial1
#define RADIO_SERIAL    Serial2
#define PC_SERIAL       Serial

#pragma pack(push, 1)
struct TelemetryPacket {
  uint8_t  header1;
  uint8_t  header2;
  uint8_t  rover_id;
  int32_t  lat;
  int32_t  lon;
  int32_t  alt_mm;
  uint8_t  fix_status;
  uint16_t h_acc_mm;
  uint16_t speed_01kmh;
  uint16_t heading_01deg;
  int16_t  pitch_01deg;
  int16_t  roll_01deg;
  int16_t  rel_N_cm;
  int16_t  rel_E_cm;
  uint8_t  checksum;
};
#pragma pack(pop)

TelemetryPacket inPacket;
uint8_t rxBuffer[sizeof(TelemetryPacket)];
size_t rxIndex = 0;

uint8_t calculateChecksum(uint8_t *data, size_t len) {
  uint8_t crc = 0;
  for (size_t i = 0; i < len; i++) crc ^= data[i];
  return crc;
}

void processRoverPacket(const TelemetryPacket& pkt) {
  // Format data menjadi JSON String dan kirim via USB Serial ke Web Dashboard Laptop
  PC_SERIAL.print(F("{\"id\":"));
  PC_SERIAL.print(pkt.rover_id);
  PC_SERIAL.print(F(",\"lat\":"));
  PC_SERIAL.print(pkt.lat / 1e7, 7);
  PC_SERIAL.print(F(",\"lon\":"));
  PC_SERIAL.print(pkt.lon / 1e7, 7);
  PC_SERIAL.print(F(",\"alt\":"));
  PC_SERIAL.print(pkt.alt_mm / 1000.0, 3);
  PC_SERIAL.print(F(",\"fix\":"));
  PC_SERIAL.print(pkt.fix_status);
  PC_SERIAL.print(F(",\"hAcc_cm\":"));
  PC_SERIAL.print(pkt.h_acc_mm / 10.0, 1);
  PC_SERIAL.print(F(",\"speed_kmh\":"));
  PC_SERIAL.print(pkt.speed_01kmh / 10.0, 1);
  PC_SERIAL.print(F(",\"heading\":"));
  PC_SERIAL.print(pkt.heading_01deg / 10.0, 1);
  PC_SERIAL.print(F(",\"pitch\":"));
  PC_SERIAL.print(pkt.pitch_01deg / 10.0, 1);
  PC_SERIAL.print(F(",\"roll\":"));
  PC_SERIAL.print(pkt.roll_01deg / 10.0, 1);
  PC_SERIAL.print(F(",\"relN_m\":"));
  PC_SERIAL.print(pkt.rel_N_cm / 100.0, 2);
  PC_SERIAL.print(F(",\"relE_m\":"));
  PC_SERIAL.print(pkt.rel_E_cm / 100.0, 2);
  PC_SERIAL.println(F("}"));
}

void setup() {
  PC_SERIAL.begin(115200);       // USB Serial ke Laptop Web Dashboard
  RTK_BASE_SERIAL.begin(38400);  // Baudrate ZED-F9P Base
  RADIO_SERIAL.begin(57600);    // Baudrate Radio Telemetry
}

void loop() {
  // 1. Downlink: Forward data koreksi RTCM3 dari Base ZED-F9P ke Radio Telemetry
  while (RTK_BASE_SERIAL.available()) {
    uint8_t b = RTK_BASE_SERIAL.read();
    RADIO_SERIAL.write(b);
  }

  // 2. Uplink: Terima paket telemetri dari Coastal Drifter via Radio Telemetry
  while (RADIO_SERIAL.available()) {
    uint8_t b = RADIO_SERIAL.read();

    if (rxIndex == 0 && b != 0xAA) continue;
    if (rxIndex == 1 && b != 0xBB) { rxIndex = 0; continue; }

    rxBuffer[rxIndex++] = b;

    if (rxIndex >= sizeof(TelemetryPacket)) {
      memcpy(&inPacket, rxBuffer, sizeof(TelemetryPacket));
      uint8_t calcCrc = calculateChecksum(rxBuffer, sizeof(TelemetryPacket) - 1);

      if (calcCrc == inPacket.checksum) {
        processRoverPacket(inPacket); // Packet Valid! Stream JSON ke Web Dashboard
      }
      rxIndex = 0;
    }
  }
}
