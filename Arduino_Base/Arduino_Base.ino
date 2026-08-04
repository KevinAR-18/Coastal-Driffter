#include <Arduino.h>
#include <SoftwareSerial.h>

// ============================================================================
// COASTAL DRIFFTER - BASE STATION FIRMWARE
// ============================================================================
// ARSITEKTUR: RTCM3 mengalir di HARDWARE, tidak lewat Arduino.
//
//   BASE                                      ROVER
//   ZED-F9P UART2 TX --> XBee DIN  ~~radio~~> XBee DOUT --> ZED-F9P UART2 RX
//     (RTCM3 1005/1074/1094)                    (koreksi masuk)
//                                             ZED-F9P UART2 TX --> XBee DIN
//   XBee DOUT --> pin 2 (RX)      <~~radio~~    (UBX NAV-PVT + RELPOSNED)
//   Arduino USB --> Dashboard (JSON @115200)
//
// Karena socket XBee tersambung ke UART2 ZED-F9P, Arduino Base HANYA perlu
// mendengarkan uplink UBX dari Rover. Tidak ada relay RTCM3, tidak ada
// SoftwareSerial kedua, dan Base GNSS tidak perlu disambung ke Arduino.
//
// Base GNSS harus sudah dikunci TMODE3 mode 2 (Fixed) via u-center, dan
// koordinatnya diisi di BASE_LAT_1E7 / BASE_LON_1E7 / BASE_ALT_MM di bawah.
// ============================================================================

// XBee DOUT -> pin 2. Pin 3 hanya placeholder TX (tidak dipakai, receive-only).
SoftwareSerial radioSerial(2, 3);
#define PC_SERIAL Serial

// Baud harus SAMA di: UART2 Base, UART2 Rover, dan XBee BD (kedua sisi).
#define RADIO_BAUD 19200

// ----------------------------------------------------------------------------
// POSISI BASE — HASIL SURVEY-IN, BUKAN PLACEHOLDER
// ----------------------------------------------------------------------------
// Isi setelah Fase 2 Tahap B (TMODE3 Fixed Mode):
//   1. Jalankan Survey-In, tunggu UBX-NAV-SVIN valid=1 && active=0
//   2. Catat meanX/Y/Z + meanXHP/YHP/ZHP, masukkan ke TMODE3 mode 2
//   3. Baca lat/lon/alt hasilnya di u-center, tulis di sini
//
// Selama BASE_CONFIGURED 0, dashboard menampilkan "Base: NO DATA" dan TIDAK
// menggambar marker Base. Ini disengaja: lebih baik tidak ada titik daripada
// titik palsu yang tampak sah.
#define BASE_CONFIGURED 0
#define BASE_LAT_1E7 0L
#define BASE_LON_1E7 0L
#define BASE_ALT_MM  0L

// Rover dianggap hilang kalau tidak ada UBX valid selama ini.
#define LINK_TIMEOUT_MS 3000UL

// ----------------------------------------------------------------------------
struct RoverData {
  int32_t  lat, lon, alt_mm;
  uint8_t  fix;
  uint32_t hAcc_mm;
  int32_t  gSpeed_mms, headMot_1e5;
  int32_t  relN_cm, relE_cm;   // int32: baseline >5 km overflow di int16 (+-327 m)
};

RoverData rover = { 0, 0, 0, 0, 0, 0, 0, 0, 0 };

unsigned long lastJsonTime   = 0;
unsigned long lastRoverPacket = 0;
bool roverSeen = false;

// ----------------------------------------------------------------------------
// UBX STATE MACHINE
// ----------------------------------------------------------------------------
enum UbxState {
  UBX_WAIT_SYNC1, UBX_WAIT_SYNC2, UBX_WAIT_CLASS, UBX_WAIT_ID,
  UBX_WAIT_LEN1, UBX_WAIT_LEN2, UBX_PAYLOAD, UBX_WAIT_CKA, UBX_WAIT_CKB
};

// NAV-PVT payload 92 B, NAV-RELPOSNED 64 B. 100 B cukup untuk keduanya.
#define UBX_BUF_SIZE 100

struct UbxParser {
  UbxState state;
  uint8_t  uClass, uId;
  uint16_t len, index;
  uint8_t  buf[UBX_BUF_SIZE];
  uint8_t  ckA, ckB;
};

UbxParser roverParser = { UBX_WAIT_SYNC1 };

int32_t parseLong(uint8_t *b) {
  return (int32_t)((uint32_t)b[0] | ((uint32_t)b[1] << 8) |
                   ((uint32_t)b[2] << 16) | ((uint32_t)b[3] << 24));
}

uint32_t parseULong(uint8_t *b) {
  return ((uint32_t)b[0] | ((uint32_t)b[1] << 8) |
          ((uint32_t)b[2] << 16) | ((uint32_t)b[3] << 24));
}

bool processUbxByte(UbxParser &px, uint8_t c, uint8_t &outClass, uint8_t &outId) {
  switch (px.state) {
    case UBX_WAIT_SYNC1:
      if (c == 0xB5) px.state = UBX_WAIT_SYNC2;
      break;
    case UBX_WAIT_SYNC2:
      // 0xB5 beruntun harus tetap dianggap kandidat sync1, kalau tidak satu
      // byte 0xB5 nyasar sebelum frame valid akan membuang frame itu.
      if (c == 0x62)      px.state = UBX_WAIT_CLASS;
      else if (c == 0xB5) px.state = UBX_WAIT_SYNC2;
      else                px.state = UBX_WAIT_SYNC1;
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
      if (px.len > UBX_BUF_SIZE) px.state = UBX_WAIT_SYNC1;
      else if (px.len == 0)      px.state = UBX_WAIT_CKA;
      else                       px.state = UBX_PAYLOAD;
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

void parseNavPvt(UbxParser &px, RoverData &d) {
  if (px.len < 92) return;
  uint8_t *p  = px.buf;
  uint8_t  ft = p[20];
  uint8_t  cs = (p[21] >> 6) & 0x03;   // carrSoln: 0=none, 1=float, 2=fix

  if (cs == 2)      d.fix = 4;
  else if (cs == 1) d.fix = 5;
  else if (ft == 3) d.fix = 1;
  else              d.fix = 0;

  d.lon         = parseLong(&p[24]);
  d.lat         = parseLong(&p[28]);
  d.alt_mm      = parseLong(&p[36]);
  d.hAcc_mm     = parseULong(&p[40]);  // 4 byte penuh: 2 byte wrap di 65 m
  d.gSpeed_mms  = parseLong(&p[60]);
  d.headMot_1e5 = parseLong(&p[64]);
}

void parseRelposned(UbxParser &px, RoverData &d) {
  if (px.len < 40) return;
  d.relN_cm = parseLong(&px.buf[8]);
  d.relE_cm = parseLong(&px.buf[12]);
}

// ----------------------------------------------------------------------------
// JSON STREAM
// ----------------------------------------------------------------------------
void sendJson() {
  bool linkOk = roverSeen && (millis() - lastRoverPacket < LINK_TIMEOUT_MS);

  PC_SERIAL.print(F("{\"base\":{\"valid\":"));
  PC_SERIAL.print(BASE_CONFIGURED);
  PC_SERIAL.print(F(",\"lat\":"));
  PC_SERIAL.print(BASE_LAT_1E7 / 1e7, 7);
  PC_SERIAL.print(F(",\"lon\":"));
  PC_SERIAL.print(BASE_LON_1E7 / 1e7, 7);
  PC_SERIAL.print(F(",\"alt\":"));
  PC_SERIAL.print(BASE_ALT_MM / 1000.0, 3);

  PC_SERIAL.print(F("},\"rover\":{\"id\":1,\"link_ok\":"));
  PC_SERIAL.print(linkOk ? 1 : 0);
  PC_SERIAL.print(F(",\"lat\":"));
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
  // mm/s -> km/h: x3.6/1000 = /277.78. Pembagi 100000 (versi lama) = 10x kekecilan.
  PC_SERIAL.print((rover.gSpeed_mms * 36) / 10000.0, 2);
  PC_SERIAL.print(F(",\"heading\":"));
  int32_t h = rover.headMot_1e5 / 100000;
  if (h < 0) h += 360;
  PC_SERIAL.print(h);
  PC_SERIAL.print(F(",\"pitch\":0.0,\"roll\":0.0"));  // IMU butuh MCU di Rover (Fase 8)
  PC_SERIAL.print(F(",\"relN_m\":"));
  PC_SERIAL.print(rover.relN_cm / 100.0, 2);
  PC_SERIAL.print(F(",\"relE_m\":"));
  PC_SERIAL.print(rover.relE_cm / 100.0, 2);
  PC_SERIAL.println(F("}}"));
}

// ----------------------------------------------------------------------------
void setup() {
  PC_SERIAL.begin(115200);
  radioSerial.begin(RADIO_BAUD);
  radioSerial.listen();   // satu-satunya instance: listen sekali di setup
}

void loop() {
  // Uplink UBX dari Rover via XBee
  while (radioSerial.available()) {
    uint8_t b = radioSerial.read();
    uint8_t cls = 0, id = 0;
    if (processUbxByte(roverParser, b, cls, id)) {
      if (cls == 0x01 && id == 0x07) {
        parseNavPvt(roverParser, rover);
        lastRoverPacket = millis();
        roverSeen = true;
      } else if (cls == 0x01 && id == 0x3C) {
        parseRelposned(roverParser, rover);
        lastRoverPacket = millis();
        roverSeen = true;
      }
    }
  }

  // JSON 1 Hz — uplink Rover juga 1 Hz, tidak ada gunanya lebih cepat
  if (millis() - lastJsonTime >= 1000) {
    lastJsonTime = millis();
    sendJson();
  }
}
