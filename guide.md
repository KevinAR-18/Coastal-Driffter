# Coastal Driffter — Panduan Setup Lengkap

## Daftar Isi
1. [Arsitektur Sistem](#1-arsitektur-sistem)
2. [Hardware & Wiring](#2-hardware--wiring)
3. [Konfigurasi u-center](#3-konfigurasi-u-center)
4. [XBee — Pre-Paired (ArduSimple Kit)](#4-xbee--pre-paired-ardusimple-kit)
5. [Firmware Arduino Base](#5-firmware-arduino-base)
6. [Dashboard — map_dashboard.html](#6-dashboard)
7. [Troubleshooting](#7-troubleshooting)

---

## 1. Arsitektur Sistem

### Mode 1 — NMEA Rover-Only (tanpa Arduino Base)

```
Rover simpleRTK2B (NMEA) → XBee  ~~~wireless~~~  XBee → Arduino (passthrough) → USB → Dashboard @ 38400
```

| Baud Serial | Format Data | Yang Muncul di Map | Dibutuhkan |
|---|---|---|---|
| **38400** | NMEA (`$GNRMC`, `$GNGGA`) | **1 titik** (Rover) | Rover GNSS + XBee + baterai |

Program passthrough Arduino (hanya untuk uji coba):
```cpp
void setup() { Serial.begin(38400); }
void loop() {
  while (Serial.available()) { Serial.write(Serial.read()); }
  delay(100);
}
```

### Mode 2 — Full System (dengan Arduino Base)

```
                    DOWNLINK: RTCM3 koreksi
     Base GNSS (UBX) → Arduino pin 4,5 ──→ XBee pin 2,8 ~~~wireless~~~ XBee → Rover GNSS
                                                                          │
                    UPLINK: posisi Rover (UBX)                           │
     Base GNSS (UBX) ← Arduino pin 4,5 ←── XBee pin 2,8 ←~~wireless~~← XBee ← Rover GNSS
                          │
                     Merge JSON
                          │
                    USB Serial @ 115200
                          │
                     Dashboard HTML
```

| Baud Serial | Format Data | Yang Muncul di Map | Dibutuhkan |
|---|---|---|---|
| **115200** | JSON `{"base":{...},"rover":{...}}` | **2 titik** (Base + Rover) + baseline | Base GNSS + Rover GNSS + 2 XBee + Arduino UNO |

Link radio sepenuhnya ditangani XBee — **tidak ada kabel antara Base dan Rover**.

---

## 2. Hardware & Wiring

### Daftar Komponen

| Qty | Komponen |
|---|---|
| 2x | Arduino UNO (Base: wajib, Rover: opsional hanya untuk Mode 1) |
| 2x | simpleRTK2B (u-blox ZED-F9P) — 1 Base + 1 Rover |
| 2x | XBee modul radio |
| 2x | Antena GNSS |
| 1x | Power bank / baterai (Rover) |
| — | Kabel jumper male-to-female |

### Wiring Rover (Mode 2 — tanpa Arduino)

```
simpleRTK2B TX ──→ XBee RX (pin 2 pada shield)
simpleRTK2B RX ──→ XBee TX (pin 8 pada shield)
XBee          ──→ radio link (wireless)

Power: baterai 5V ke simpleRTK2B + XBee
```

> ⚠️ XBee hanya 3.3V! Jangan sambung langsung ke 5V — gunakan shield simpleRTK2B yang sudah menyediakan regulator.

### Wiring Base Station (Mode 2 — dengan Arduino)

| Komponen | Pin Arduino UNO | Bypass Shield? |
|---|---|---|
| Base GNSS **TX** | Pin **4** | Ya — jumper langsung |
| Base GNSS **RX** | Pin **5** | Ya — jumper langsung |
| XBee **TX** | Pin **2** | Ya — jumper langsung |
| XBee **RX** | Pin **8** | Ya — jumper langsung |
| Arduino USB | PC (Dashboard) | — |

Mengapa bypass shield untuk Base? Pin 0 & 1 Arduino UNO sudah dipakai untuk komunikasi USB serial ke PC. Base GNSS harus terhubung lewat SoftwareSerial (pin 4,5), bukan melalui shield standard (yang menggunakan pin 0,1).

---

## 3. Konfigurasi u-center

Download u-center dari https://www.u-blox.com/en/product/u-center

### 3a. Rover simpleRTK2B

| Menu | Setting | Keterangan |
|---|---|---|
| **View → Configuration View → PRT (Ports)** | Target: 1 - UART1 | |
| | Protocol in: **0+1 - UBX+NMEA** | |
| | Protocol out: **0+1 - UBX+NMEA** | |
| | Baudrate: **38400** | |
| | Databits: 8, Stop bits: 1, Parity: None | |
| **MSG (Messages)** | UBX-NAV-PVT (kls 0x01, ID 0x07) → UART1 → ✅ Enable, Rate: **1** | Data posisi, kecepatan, heading, fix status |
| **MSG (Messages)** | UBX-NAV-RELPOSNED (kls 0x01, ID 0x3C) → UART1 → ✅ Enable, Rate: **1** | Jarak relatif North/East terhadap Base |
| **Receiver → Action → Save Config** | ⚠️ WAJIB! | Tanpa ini, konfigurasi hilang setelah power off |

### 3b. Base simpleRTK2B

| Menu | Setting | Keterangan |
|---|---|---|
| **PRT (Ports)** | Target: 1 - UART1 | |
| | Protocol in: **0+1 - UBX+NMEA** | |
| | Protocol out: **0+1 - UBX+NMEA** | |
| | Baudrate: **38400** | |
| | Databits: 8, Stop bits: 1, Parity: None | |
| **MSG (Messages)** | UBX-NAV-PVT → UART1 → ✅ Enable, Rate: **1** | Data posisi Base |
| **TMODE3 (Time Mode 3)** | Mode: **1 - Survey-In** | Base akan survei posisinya sendiri |
| | Minimum Observation Time: **60** (detik) | Lama minimal survey — bisa diperbesar untuk akurasi lebih tinggi |
| | Required Position Accuracy: **3.0** (meter) | Akurasi posisi yang diinginkan — bisa dikurangi ke 1.0 jika antena di tempat ideal |
| **Receiver → Action → Save Config** | ⚠️ WAJIB! | |

#### Indikator LED Base setelah dinyalakan:

| Kondisi | LED Base |
|---|---|
| Boot / Survey-In berjalan | Berkedip |
| Survey-In selesai, kirim RTCM3 | Pola tetap / berubah |
| Rover terima RTCM3 | Rover akan menampilkan `fix=4` (RTK FIX) di dashboard |

Waktu Survey-In: **60-120 detik pertama** setelah dinyalakan, Base akan mengukur posisi presisinya. Pastikan antena Base di tempat statis (tidak bergerak).

---

## 4. XBee — Pre-Paired (ArduSimple Kit)

XBee yang disertakan dalam kit ArduSimple **sudah dipasangkan dari pabrik** (PAN ID, Channel, Destination sama). Tidak perlu konfigurasi tambahan.

Apabila nanti bermasalah (terindikasi komunikasi putus), verifikasi di XCTU:

| Setting | Rover XBee | Base XBee |
|---|---|---|
| PAN ID | Harus sama | Harus sama |
| CH (Channel) | Harus sama | Harus sama |
| DH/DL (Destination) | MAC Base | MAC Rover |
| BD (Baud Rate) | 9600 | 9600 |

---

## 5. Firmware Arduino Base

### Upload — Hanya untuk Base Station

1. Buka Arduino IDE
2. **Tools → Board**: Arduino UNO
3. **Tools → Port**: Pilih COM port Arduino Base
4. Buka file `Arduino_Base/Arduino_Base.ino`
5. Klik **Upload**

### Rangkuman Kode

| Pin | Fungsi | Baud |
|---|---|---|
| USB (Serial) | Output JSON ke PC | 115200 |
| Pin 4(RX), 5(TX) | UBX data dari Base GNSS | 38400 |
| Pin 2(RX), 8(TX) | UBX data dari Rover GNSS via XBee | 9600 |

### Struktur Program

```
loop():
  1. gnssSerial.listen() → baca Base GNSS (UBX-NAV-PVT), relay RTCM3 ke Rover via XBee
  2. radioSerial.listen() → baca Rover GNSS (UBX-NAV-PVT + NAV-RELPOSNED) via XBee
  3. sendJson() setiap 200ms → merge base + rover → JSON stream ke USB Serial
```

Output JSON contoh:
```json
{"base":{"lat":-6.1753924,"lon":106.8271532,"alt":15.420},"rover":{"id":1,"lat":-6.1754100,"lon":106.8272100,"alt":15.500,"fix":4,"hAcc_cm":1.2,"speed_kmh":14.5,"heading":128,"pitch":0.0,"roll":0.0,"relN_m":42.15,"relE_m":18.70}}
```

### Tidak Perlu Arduino Rover

Untuk Mode 2, Rover tidak menggunakan Arduino sama sekali. File `Arduino_Rover/Arduino_Rover.ino` adalah referensi untuk implementasi masa depan — **tidak perlu di-upload**.

---

## 6. Dashboard — map_dashboard.html

### Browser
- **Google Chrome** atau **Microsoft Edge** (satu-satunya browser yang mendukung Web Serial API)
- **Gunakan HTTPS atau localhost** — Web Serial API memerlukan konteks aman. Bisa buka file HTML langsung (file://), Chrome mengizinkan Web Serial pada file lokal.

### Fitur

| Fitur | Keterangan |
|---|---|
| Baud Selector | **38400** untuk Mode NMEA (Rover-only) / **115200** untuk Mode JSON (Base+Rover) |
| 🔌 Hubungkan ke Serial | Pilih COM port lalu mulai streaming |
| 🔴 Record / ⏹️ Stop | Simpan log ke memori browser |
| 📥 Export CSV | Data per titik: timestamp, lat, lon, alt, fix, speed, heading |
| 🗺️ Export GPX | Format track standar GPS |
| 📍 Export KML | Untuk Google Earth |
| 🗑️ Clear Track | Bersihkan lintasan + log |
| **Base marker** 📌| Titik diam Base Station (hanya di Mode 2) |
| **Rover marker** 🚤| Titik bergerak + trail line posisi Rover |
| **Baseline line** | Garis penghubung Base-Rover + jarak real-time (hanya Mode 2) |
| **Fix Status** | RTK FIX / RTK FLOAT / DGPS / SINGLE / NO FIX |
| **Total Jarak Rover** | Akumulasi jarak lintasan sejak titik pertama |

### Cara Pakai

#### Mode NMEA (test Rover saja, tanpa Arduino Base)
1. Sambung XBee Rover ke Arduino UNO (pin 0=RX, 1=TX)
2. Upload program passthrough (`Serial.write(Serial.read())`) ke Arduino
3. Nyalakan Rover GNSS, biarkan XBee menyala
4. Buka `map_dashboard.html` di Chrome
5. Pilih baud **38400**
6. Klik **🔌 Hubungkan ke Serial** → pilih COM port Arduino
7. Peta auto-center ke posisi Rover, tracking dimulai

#### Mode JSON (full system, dengan Arduino Base)
1. Sambung Base GNSS ke pin 4,5 Arduino Base
2. Sambung XBee ke pin 2,8 Arduino Base
3. Upload `Arduino_Base/Arduino_Base.ino`
4. Nyalakan Base GNSS + Rover GNSS (keduanya dengan XBee menyala)
5. Tunggu ~60 detik sampai Base selesai Survey-In
6. Buka `map_dashboard.html` di Chrome
7. Pilih baud **115200**
8. Klik **🔌 Hubungkan ke Serial** → pilih COM port Arduino Base
9. Muncul 2 titik: Base + Rover dengan baseline line

---

## 7. Troubleshooting

### Dashboard tidak bisa connect

| Masalah | Solusi |
|---|---|
| "Failed to execute 'requestPort'" | Web Serial hanya berfungsi di Chrome/Edge, dan di konteks aman (HTTPS/file://). Pastikan port tidak sedang dibuka oleh Arduino IDE Serial Monitor. |
| Port terdaftar tapi timeout | Cek baud di dashboard (select dropdown) sesuai dengan firmware — 38400 untuk NMEA, 115200 untuk JSON |
| Port tidak muncul | Cabut-pasang USB Arduino. Cek Device Manager apakah driver CH340 terinstal. Pastikan Arduino sudah di-upload firmware yang benar. |

### Hanya 1 titik di map

| Masalah | Solusi |
|---|---|
| Mode 38400 — normal | Mode NMEA hanya menampilkan Rover. Untuk dua titik, gunakan Mode 2 (baud 115200). |
| Mode 115200 tapi hanya Base | Rover GNSS belum menyala / XBee tidak terhubung. Cek XBee pairing via XCTU (PAN ID, Channel). |
| Mode 115200 tapi hanya Rover | Base GNSS belum menyala / wiring pin 4,5 bermasalah. Dashboard akan fallback ke koordinat hardcode Monas. |

### NO FIX / gak dapat lock satelit

| Masalah | Solusi |
|---|---|
| NO FIX terus-menerus | Pastikan antena GNSS di luar ruangan (clear sky), tidak di dalam. Tunggu 1-2 menit untuk cold start. |
| Tetap SINGLE (fix=1) tidak naik ke RTK FIX | Base belum menyala / Survey-In belum selesai / XBee tidak terhubung. Cek TMODE3 di Base (u-center), pastikan sudah Save Config. |
| TDOP/GDOP tinggi | Lingkungan banyak gedung/pohon — pindahkan antena ke tempat lebih terbuka. |

### Wiring & Hardware

| Masalah | Solusi |
|---|---|
| Upload Arduino Base gagal | Cabut kabel jumper dari pin 0 dan 1 sebelum upload |
| simpleRTK2B tidak menyala | Cek kabel power — shield butuh 5V dengan arus cukup (>200mA). LED merah pada board harus menyala. |
| XBee tidak berkomunikasi | Pastikan kedua XBee dalam jarak jangkauan (line-of-sight lebih baik). Di dalam ruangan, XBee jarak terbatas. |

### Dashboard / Map

| Masalah | Solusi |
|---|---|
| Map polos (tile tidak muncul) | Butuh koneksi internet — Leaflet tile dari OpenStreetMap CDN. |
| Data NMEA kadang loncat-loncat | Normal — NMEA default tanpa RTK memiliki jitter 1-3 meter. Gunakan mode JSON dengan RTK untuk akurasi cm. |
| Export gagal / file rusak | Cek browser permission untuk download. |

---

## Ringkasan Cepat

```
MODE NMEA (tes): Rover GNSS → XBee ~~~ XBee → Arduino passthrough → USB → Dashboard @ 38400
MODE FULL:       Base GNSS + Rover GNSS → 2x XBee ~~~ → Arduino Base → JSON → Dashboard @ 11500
```

| | Mode NMEA | Mode JSON |
|---|---|---|
| **Baud** | 38400 | 115200 |
| **Arduino Base** | Passthrough simpel | Arduino_Base.ino |
| **Arduino Rover** | Tidak ada | Tidak ada |
| **u-center Rover** | UBX+NMEA, NAV-PVT | UBX+NMEA, NAV-PVT + NAV-RELPOSNED |
| **u-center Base** | — | UBX+NMEA, NAV-PVT, TMODE3 Survey-In |
| **Map** | 1 titik (Rover) | 2 titik (Base + Rover) |
| **Akurasi** | 1.5m | 1-2cm (RTK FIX) |

---

*Coastal Driffter Setup Guide — revisi 2026.*
