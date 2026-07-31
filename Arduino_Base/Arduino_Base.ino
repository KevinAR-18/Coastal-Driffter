#include <Arduino.h>

// ============================================================================
// COASTAL DRIFFTER - BASE STATION FIRMWARE (Dual-GNSS Base & Rover Telemetry)
// ============================================================================
// Serial1: Terhubung ke simpleRTK2B Base ZED-F9P (RTCM3 Output & UBX-NAV-PVT)
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

// Data Koordinat Live Base Station (Default Monas / Static Base)
int32_t base_lat = -61753924;   // -6.1753924 deg
int32_t base_lon = 1068271532;  // 106.8271532 deg
int32_t base_alt_mm = 15420;    // 15.420 m

// ----------------------------------------------------------------------------
// UBX-NAV-PVT PARSER UNTUK BASE STATION ZED-F9P
// ----------------------------------------------------------------------------
enum UbxState {
  UBX_WAIT_SYNC1, UBX_WAIT_SYNC2, UBX_WAIT_CLASS, UBX_WAIT_ID,
  UBX_WAIT_LEN1, UBX_WAIT_LEN2, UBX_PAYLOAD, UBX_WAIT_CKA, UBX_WAIT_CKB
};

UbxState baseUbxState = UBX_WAIT_SYNC1;
uint8_t baseUbxClass = 0, baseUbxId = 0;
uint16_t baseUbxLen = 0, baseUbxIndex = 0;
uint8_t baseUbxBuf[128];
uint8_t baseCkA = 0, baseCkB = 0;

int32_t parseLong(uint8_t *b) {
  return (int32_t)((uint32_t)b[0] | ((uint32_t)b[1] << 8) | ((uint32_t)b[2] << 16) | ((uint32_t)b[3] << 24));
}

void parseBaseUbxPayload(uint8_t uClass, uint8_t uId, uint8_t *payload, uint16_t len) {
  // UBX-NAV-PVT (Class 0x01, ID 0x07)
  if (uClass == 0x01 && uId == 0x07 && len >= 92) {
    base_lon = parseLong(&payload[24]);    // Deg * 1e7
    base_lat = parseLong(&payload[28]);    // Deg * 1e7
    base_alt_mm = parseLong(&payload[36]); // mm MSL
  }
}

void processBaseByteUBX(uint8_t c) {
  switch (baseUbxState) {
    case UBX_WAIT_SYNC1:
      if (c == 0xB5) baseUbxState = UBX_WAIT_SYNC2;
      break;
    case UBX_WAIT_SYNC2:
      if (c == 0x62) baseUbxState = UBX_WAIT_CLASS;
      else baseUbxState = UBX_WAIT_SYNC1;
      break;
    case UBX_WAIT_CLASS:
      baseUbxClass = c; baseCkA = c; baseCkB = c;
      baseUbxState = UBX_WAIT_ID;
      break;
    case UBX_WAIT_ID:
      baseUbxId = c; baseCkA += c; baseCkB += baseCkA;
      baseUbxState = UBX_WAIT_LEN1;
      break;
    case UBX_WAIT_LEN1:
      baseUbxLen = c; baseCkA += c; baseCkB += baseCkA;
      baseUbxState = UBX_WAIT_LEN2;
      break;
    case UBX_WAIT_LEN2:
      baseUbxLen |= ((uint16_t)c << 8); baseCkA += c; baseCkB += baseCkA;
      baseUbxIndex = 0;
      if (baseUbxLen > sizeof(baseUbxBuf)) baseUbxState = UBX_WAIT_SYNC1;
      else if (baseUbxLen == 0) baseUbxState = UBX_WAIT_CKA;
      else baseUbxState = UBX_PAYLOAD;
      break;
    case UBX_PAYLOAD:
      baseUbxBuf[baseUbxIndex++] = c; baseCkA += c; baseCkB += baseCkA;
      if (baseUbxIndex >= baseUbxLen) baseUbxState = UBX_WAIT_CKA;
      break;
    case UBX_WAIT_CKA:
      if (c == baseCkA) baseUbxState = UBX_WAIT_CKB;
      else baseUbxState = UBX_WAIT_SYNC1;
      break;
    case UBX_WAIT_CKB:
      if (c == baseCkB) parseBaseUbxPayload(baseUbxClass, baseUbxId, baseUbxBuf, baseUbxLen);
      baseUbxState = UBX_WAIT_SYNC1;
      break;
  }
}

uint8_t calculateChecksum(uint8_t *data, size_t len) {
  uint8_t crc = 0;
  for (size_t i = 0; i < len; i++) crc ^= data[i];
  return crc;
}

void processRoverPacket(const TelemetryPacket& pkt) {
  // Format JSON Stream ganda (base + rover) ke Laptop Web Dashboard
  PC_SERIAL.print(F("{\"base\":{\"lat\":"));
  PC_SERIAL.print(base_lat / 1e7, 7);
  PC_SERIAL.print(F(",\"lon\":"));
  PC_SERIAL.print(base_lon / 1e7, 7);
  PC_SERIAL.print(F(",\"alt\":"));
  PC_SERIAL.print(base_alt_mm / 1000.0, 3);
  PC_SERIAL.print(F("},\"rover\":{\"id\":"));
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
  PC_SERIAL.println(F("}}"));
}

void setup() {
  PC_SERIAL.begin(115200);       // USB Serial ke Laptop Web Dashboard
  RTK_BASE_SERIAL.begin(38400);  // Baudrate ZED-F9P Base
  RADIO_SERIAL.begin(57600);    // Baudrate Radio Telemetry
}

void loop() {
  // 1. Downlink: Forward data koreksi RTCM3 & Parse UBX Base dari ZED-F9P Base ke Radio
  while (RTK_BASE_SERIAL.available()) {
    uint8_t b = RTK_BASE_SERIAL.read();
    RADIO_SERIAL.write(b);   // Downlink RTCM3 ke Rover
    processBaseByteUBX(b);  // Parse Base Live GNSS Location
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
        processRoverPacket(inPacket); // Packet Valid! Stream Dual-GNSS JSON ke PC
      }
      rxIndex = 0;
    }
  }
}
