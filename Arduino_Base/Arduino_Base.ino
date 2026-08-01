#include <Arduino.h>
#include <SoftwareSerial.h>

// ============================================================================
// COASTAL DRIFFTER - BASE STATION FIRMWARE
// Rover tanpa Arduino — raw UBX dari simpleRTK2B Rover via XBee
// ============================================================================
// SoftwareSerial (pin 4=RX, 5=TX): Terhubung ke simpleRTK2B Base ZED-F9P
// SoftwareSerial (pin 2=RX, 8=TX): Terhubung ke XBee Radio Telemetry
//   - Menerima raw UBX dari Rover GNSS via XBee
//   - Mengirim RTCM3 koreksi ke Rover via XBee
// HW Serial (USB): ke PC Dashboard (JSON Stream @ 115200 bps)
// ============================================================================

SoftwareSerial gnssSerial(4, 5);
SoftwareSerial radioSerial(2, 8);
#define PC_SERIAL Serial

// ----------------------------------------------------------------------------
// GNSS DATA
// ----------------------------------------------------------------------------
struct GnssData {
  int32_t  lat, lon, alt_mm;
  uint8_t  fix;
  uint16_t hAcc_mm;
  int32_t  gSpeed_mms, headMot_1e5;
  int32_t  relN_cm, relE_cm;
};

GnssData base  = { -61753924, 1068271532, 15420, 0, 0, 0, 0, 0, 0 };
GnssData rover = { 0 };

unsigned long lastJsonTime = 0;

// ----------------------------------------------------------------------------
// UBX STATE MACHINE
// ----------------------------------------------------------------------------
enum UbxState {
  UBX_WAIT_SYNC1, UBX_WAIT_SYNC2, UBX_WAIT_CLASS, UBX_WAIT_ID,
  UBX_WAIT_LEN1, UBX_WAIT_LEN2, UBX_PAYLOAD, UBX_WAIT_CKA, UBX_WAIT_CKB
};

struct UbxParser {
  UbxState state;
  uint8_t  uClass, uId;
  uint16_t len, index;
  uint8_t  buf[128];
  uint8_t  ckA, ckB;
};

UbxParser baseParser  = { UBX_WAIT_SYNC1 };
UbxParser roverParser = { UBX_WAIT_SYNC1 };

int32_t parseLong(uint8_t *b) {
  return (int32_t)((uint32_t)b[0] | ((uint32_t)b[1] << 8) | ((uint32_t)b[2] << 16) | ((uint32_t)b[3] << 24));
}

bool processUbxByte(UbxParser &px, uint8_t c, uint8_t &outClass, uint8_t &outId) {
  switch (px.state) {
    case UBX_WAIT_SYNC1:
      if (c == 0xB5) px.state = UBX_WAIT_SYNC2;
      break;
    case UBX_WAIT_SYNC2:
      px.state = (c == 0x62) ? UBX_WAIT_CLASS : UBX_WAIT_SYNC1;
      break;
    case UBX_WAIT_CLASS:
      px.uClass = c; px.ckA = c; px.ckB = c; px.state = UBX_WAIT_ID;
      break;
    case UBX_WAIT_ID:
      px.uId = c; px.ckA += c; px.ckB += px.ckA; px.state = UBX_WAIT_LEN1;
      break;
    case UBX_WAIT_LEN1:
      px.len = c; px.ckA += c; px.ckB += px.ckA; px.state = UBX_WAIT_LEN2;
      break;
    case UBX_WAIT_LEN2:
      px.len |= ((uint16_t)c << 8); px.ckA += c; px.ckB += px.ckA; px.index = 0;
      if (px.len > sizeof(px.buf)) px.state = UBX_WAIT_SYNC1;
      else if (px.len == 0)        px.state = UBX_WAIT_CKA;
      else                         px.state = UBX_PAYLOAD;
      break;
    case UBX_PAYLOAD:
      px.buf[px.index++] = c; px.ckA += c; px.ckB += px.ckA;
      if (px.index >= px.len) px.state = UBX_WAIT_CKA;
      break;
    case UBX_WAIT_CKA:
      px.state = (c == px.ckA) ? UBX_WAIT_CKB : UBX_WAIT_SYNC1;
      break;
    case UBX_WAIT_CKB:
      px.state = UBX_WAIT_SYNC1;
      if (c == px.ckB) {
        outClass = px.uClass;
        outId    = px.uId;
        return true;
      }
      break;
  }
  return false;
}

void parseNavPvt(UbxParser &px, GnssData &d) {
  uint8_t *p  = px.buf;
  uint8_t  ft = p[20];
  uint8_t  cs = (p[21] >> 6) & 0x03;

  if (cs == 2)      d.fix = 4;
  else if (cs == 1) d.fix = 5;
  else if (ft == 3) d.fix = 1;
  else              d.fix = 0;

  d.lon         = parseLong(&p[24]);
  d.lat         = parseLong(&p[28]);
  d.alt_mm      = parseLong(&p[36]);
  d.hAcc_mm     = (uint16_t)((uint32_t)((uint32_t)p[40] | ((uint32_t)p[41] << 8)) & 0xFFFF);
  d.gSpeed_mms  = parseLong(&p[60]);
  d.headMot_1e5 = parseLong(&p[64]);
}

void parseRelposned(UbxParser &px, GnssData &d) {
  d.relN_cm = parseLong(&px.buf[8]);
  d.relE_cm = parseLong(&px.buf[12]);
}

// ----------------------------------------------------------------------------
// JSON STREAM
// ----------------------------------------------------------------------------
void sendJson() {
  PC_SERIAL.print(F("{\"base\":{\"lat\":"));
  PC_SERIAL.print(base.lat / 1e7, 7);
  PC_SERIAL.print(F(",\"lon\":"));
  PC_SERIAL.print(base.lon / 1e7, 7);
  PC_SERIAL.print(F(",\"alt\":"));
  PC_SERIAL.print(base.alt_mm / 1000.0, 3);

  PC_SERIAL.print(F("},\"rover\":{\"id\":1,\"lat\":"));
  PC_SERIAL.print(rover.lat / 1e7, 7);
  PC_SERIAL.print(F(",\"lon\":"));
  PC_SERIAL.print(rover.lon / 1e7, 7);
  PC_SERIAL.print(F(",\"alt\":"));
  PC_SERIAL.print(rover.alt_mm / 1000.0, 3);
  PC_SERIAL.print(F(",\"fix\":"));
  PC_SERIAL.print(rover.fix);
  PC_SERIAL.print(F(",\"hAcc_cm\":"));
  PC_SERIAL.print(rover.hAcc_mm / 10.0, 1);
  PC_SERIAL.print(F(",\"speed_kmh\":"));
  PC_SERIAL.print((rover.gSpeed_mms * 36) / 100000.0, 1);
  PC_SERIAL.print(F(",\"heading\":"));
  int32_t h = rover.headMot_1e5 / 100000;
  if (h < 0) h += 360;
  PC_SERIAL.print(h);
  PC_SERIAL.print(F(",\"pitch\":0.0,\"roll\":0.0"));
  PC_SERIAL.print(F(",\"relN_m\":"));
  PC_SERIAL.print(rover.relN_cm / 100.0, 2);
  PC_SERIAL.print(F(",\"relE_m\":"));
  PC_SERIAL.print(rover.relE_cm / 100.0, 2);
  PC_SERIAL.println(F("}}"));
}

// ----------------------------------------------------------------------------
void setup() {
  PC_SERIAL.begin(115200);
  gnssSerial.begin(38400);
  radioSerial.begin(9600);
}

void loop() {
  // 1. Base GNSS — parse + relay RTCM3 ke Rover
  gnssSerial.listen();
  while (gnssSerial.available()) {
    uint8_t b = gnssSerial.read();
    radioSerial.write(b); // forward RTCM3
    uint8_t cls = 0, id = 0;
    if (processUbxByte(baseParser, b, cls, id)) {
      if (cls == 0x01 && id == 0x07)       parseNavPvt(baseParser, base);
      else if (cls == 0x01 && id == 0x3C)  parseRelposned(baseParser, base);
    }
  }

  // 2. Rover UBX via XBee
  radioSerial.listen();
  while (radioSerial.available()) {
    uint8_t b = radioSerial.read();
    uint8_t cls = 0, id = 0;
    if (processUbxByte(roverParser, b, cls, id)) {
      if (cls == 0x01 && id == 0x07)       parseNavPvt(roverParser, rover);
      else if (cls == 0x01 && id == 0x3C)  parseRelposned(roverParser, rover);
    }
  }

  // 3. JSON 5 Hz
  if (millis() - lastJsonTime >= 200) {
    lastJsonTime = millis();
    sendJson();
  }
}
