#include <Arduino.h>
#include <SoftwareSerial.h>

// ============================================================================
// COASTAL DRIFFTER - ROVER FIRMWARE (ZED-F9P Native UBX Parser + IMU Telemetry)
// ============================================================================
// HW Serial (pin 0=RX, 1=TX): Terhubung ke simpleRTK2B Rover ZED-F9P
// SoftwareSerial (pin 2=RX, 8=TX): Terhubung ke XBee Radio Telemetry
// ============================================================================

#define RTK_SERIAL Serial
SoftwareSerial radioSerial(2, 8);

// Set 1 untuk mengaktifkan data simulasi jika hardware ZED-F9P belum terhubung
#define ENABLE_UBX_SIMULATION 0

#pragma pack(push, 1)
struct TelemetryPacket {
  uint8_t  header1 = 0xAA;
  uint8_t  header2 = 0xBB;
  uint8_t  rover_id = 1;
  int32_t  lat;           // Deg * 1e7
  int32_t  lon;           // Deg * 1e7
  int32_t  alt_mm;        // mm MSL / Ellipsoid
  uint8_t  fix_status;    // 4=RTK FIX, 5=RTK FLOAT, 1=Single
  uint16_t h_acc_mm;      // mm
  uint16_t speed_01kmh;   // 0.1 km/h
  uint16_t heading_01deg;  // 0.1 deg
  int16_t  pitch_01deg;   // 0.1 deg IMU Pitch
  int16_t  roll_01deg;    // 0.1 deg IMU Roll
  int16_t  rel_N_cm;      // cm Baseline North
  int16_t  rel_E_cm;      // cm Baseline East
  uint8_t  checksum;
};
#pragma pack(pop)

TelemetryPacket telePacket;
unsigned long lastSendTime = 0;

// ----------------------------------------------------------------------------
// NATIVE UBX PARSER STATE MACHINE & DATA STRUCTURES
// ----------------------------------------------------------------------------
enum UbxState {
  UBX_WAIT_SYNC1,
  UBX_WAIT_SYNC2,
  UBX_WAIT_CLASS,
  UBX_WAIT_ID,
  UBX_WAIT_LEN1,
  UBX_WAIT_LEN2,
  UBX_PAYLOAD,
  UBX_WAIT_CKA,
  UBX_WAIT_CKB
};

UbxState ubxState = UBX_WAIT_SYNC1;
uint8_t ubxClass = 0;
uint8_t ubxId = 0;
uint16_t ubxLen = 0;
uint16_t ubxIndex = 0;
uint8_t ubxBuf[256];
uint8_t ckA = 0, ckB = 0;

uint8_t calculateChecksum(uint8_t *data, size_t len) {
  uint8_t crc = 0;
  for (size_t i = 0; i < len; i++) crc ^= data[i];
  return crc;
}

// Implementasi pembacaan integer little-endian dari buffer UBX
int32_t parseLong(uint8_t *b) {
  return (int32_t)((uint32_t)b[0] | ((uint32_t)b[1] << 8) | ((uint32_t)b[2] << 16) | ((uint32_t)b[3] << 24));
}

uint32_t parseULong(uint8_t *b) {
  return ((uint32_t)b[0] | ((uint32_t)b[1] << 8) | ((uint32_t)b[2] << 16) | ((uint32_t)b[3] << 24));
}

uint16_t parseUShort(uint8_t *b) {
  return (uint16_t)(b[0] | (b[1] << 8));
}

void parseUbxPayload(uint8_t uClass, uint8_t uId, uint8_t *payload, uint16_t len) {
  // UBX-NAV-PVT (Class 0x01, ID 0x07, Panjang Payload minimal 92 byte)
  if (uClass == 0x01 && uId == 0x07 && len >= 92) {
    uint8_t fixType = payload[20];
    uint8_t flags = payload[21];
    uint8_t carrSoln = (flags >> 6) & 0x03; // Bits 6..7: 0=No, 1=Float, 2=Fix

    // Tentukan Fix Status: 4=RTK FIX, 5=RTK FLOAT, 1=Single
    if (carrSoln == 2) {
      telePacket.fix_status = 4; // RTK FIX
    } else if (carrSoln == 1) {
      telePacket.fix_status = 5; // RTK FLOAT
    } else if (fixType == 3) {
      telePacket.fix_status = 1; // 3D Fix (Single)
    } else {
      telePacket.fix_status = 0; // No Fix
    }

    telePacket.lon = parseLong(&payload[24]);       // Deg * 1e7
    telePacket.lat = parseLong(&payload[28]);       // Deg * 1e7
    telePacket.alt_mm = parseLong(&payload[36]);    // Height MSL mm
    telePacket.h_acc_mm = (uint16_t)(parseULong(&payload[40]) & 0xFFFF); // hAcc mm

    int32_t gSpeed_mms = parseLong(&payload[60]);   // Ground Speed mm/s
    int32_t headMot_1e5 = parseLong(&payload[64]);  // Heading Motion 1e-5 deg

    // Konversi Kecepatan & Heading
    telePacket.speed_01kmh = (uint16_t)((gSpeed_mms * 36) / 10000); // mm/s -> 0.1 km/h
    int32_t head01 = headMot_1e5 / 10000;
    if (head01 < 0) head01 += 3600;
    telePacket.heading_01deg = (uint16_t)(head01 % 3600);
  }
  // UBX-NAV-RELPOSNED (Class 0x01, ID 0x3C, Panjang Payload minimal 40 byte)
  else if (uClass == 0x01 && uId == 0x3C && len >= 40) {
    int32_t relN_cm = parseLong(&payload[8]);   // relPosN cm
    int32_t relE_cm = parseLong(&payload[12]);  // relPosE cm

    telePacket.rel_N_cm = (int16_t)constrain(relN_cm, -32767, 32767);
    telePacket.rel_E_cm = (int16_t)constrain(relE_cm, -32767, 32767);
  }
}

void processIncomingByteUBX(uint8_t c) {
  switch (ubxState) {
    case UBX_WAIT_SYNC1:
      if (c == 0xB5) ubxState = UBX_WAIT_SYNC2;
      break;
    case UBX_WAIT_SYNC2:
      if (c == 0x62) ubxState = UBX_WAIT_CLASS;
      else ubxState = UBX_WAIT_SYNC1;
      break;
    case UBX_WAIT_CLASS:
      ubxClass = c;
      ckA = c; ckB = c;
      ubxState = UBX_WAIT_ID;
      break;
    case UBX_WAIT_ID:
      ubxId = c;
      ckA += c; ckB += ckA;
      ubxState = UBX_WAIT_LEN1;
      break;
    case UBX_WAIT_LEN1:
      ubxLen = c;
      ckA += c; ckB += ckA;
      ubxState = UBX_WAIT_LEN2;
      break;
    case UBX_WAIT_LEN2:
      ubxLen |= ((uint16_t)c << 8);
      ckA += c; ckB += ckA;
      ubxIndex = 0;
      if (ubxLen > sizeof(ubxBuf)) {
        ubxState = UBX_WAIT_SYNC1; // Buffer overflow safety
      } else if (ubxLen == 0) {
        ubxState = UBX_WAIT_CKA;
      } else {
        ubxState = UBX_PAYLOAD;
      }
      break;
    case UBX_PAYLOAD:
      ubxBuf[ubxIndex++] = c;
      ckA += c; ckB += ckA;
      if (ubxIndex >= ubxLen) {
        ubxState = UBX_WAIT_CKA;
      }
      break;
    case UBX_WAIT_CKA:
      if (c == ckA) ubxState = UBX_WAIT_CKB;
      else ubxState = UBX_WAIT_SYNC1;
      break;
    case UBX_WAIT_CKB:
      if (c == ckB) {
        parseUbxPayload(ubxClass, ubxId, ubxBuf, ubxLen);
      }
      ubxState = UBX_WAIT_SYNC1;
      break;
  }
}

// Pembacaan data orientasi dari IMU (Contoh / Hook interface)
void readImuSensors() {
  // Pada implementasi nyata, baca I2C sensor IMU (MPU6050/BNO055) di sini
  // telePacket.pitch_01deg = (int16_t)(pitch * 10);
  // telePacket.roll_01deg  = (int16_t)(roll * 10);
  telePacket.pitch_01deg = 15; // 1.5 deg pitch
  telePacket.roll_01deg  = -8; // -0.8 deg roll
}

void setup() {
  RTK_SERIAL.begin(38400);   // Baudrate ZED-F9P Rover
  radioSerial.begin(9600);   // Baudrate XBee Radio Telemetry
}

void loop() {
  // 1. Downlink: Forward data koreksi RTCM3 dari Radio Telemetry ke simpleRTK2B Rover
  while (radioSerial.available()) {
    uint8_t b = radioSerial.read();
    RTK_SERIAL.write(b); // Feed RTCM3 correction into ZED-F9P
  }

  // 2. Read Native UBX binary bytes from ZED-F9P UART
  while (RTK_SERIAL.available()) {
    uint8_t b = RTK_SERIAL.read();
    processIncomingByteUBX(b);
  }

  // 3. Uplink Telemetry Packet ke Base Station (5 Hz = 200 ms)
  if (millis() - lastSendTime >= 200) {
    lastSendTime = millis();

    readImuSensors();

#if ENABLE_UBX_SIMULATION
    // Mode simulasi jika hardware ZED-F9P tidak terhubung langsung
    telePacket.lat = -61753924;      // -6.1753924 Deg
    telePacket.lon = 1068271532;     // 106.8271532 Deg
    telePacket.alt_mm = 15420;       // 15.42 m
    telePacket.fix_status = 4;       // 4 = RTK FIX
    telePacket.h_acc_mm = 12;        // 1.2 cm
    telePacket.speed_01kmh = 145;    // 14.5 km/h
    telePacket.heading_01deg = 1284; // 128.4 deg
    telePacket.rel_N_cm = 4215;      // 42.15 m
    telePacket.rel_E_cm = 1870;      // 18.70 m
#endif

    // Hitung Checksum
    telePacket.checksum = calculateChecksum((uint8_t*)&telePacket, sizeof(TelemetryPacket) - 1);

    // Kirim paket telemetri nirkabel ke Base Station
    radioSerial.write((uint8_t*)&telePacket, sizeof(TelemetryPacket));
  }
}
