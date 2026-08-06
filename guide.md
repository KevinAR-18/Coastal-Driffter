# Coastal Driffter — Panduan Setup Lengkap

## Daftar Isi
1. [Arsitektur Sistem](#1-arsitektur-sistem)
2. [Hardware & Wiring](#2-hardware--wiring)
3. [Konfigurasi u-center — Base](#3-konfigurasi-u-center--base)
4. [Konfigurasi u-center — Rover](#4-konfigurasi-u-center--rover)
5. [XBee SX](#5-xbee-sx)
6. [Firmware Arduino Base](#6-firmware-arduino-base)
7. [Dashboard](#7-dashboard)
8. [Bring-Up Berlapis](#8-bring-up-berlapis)
9. [Troubleshooting](#9-troubleshooting)

---

## 1. Arsitektur Sistem

### Prinsip Utama

Socket XBee pada simpleRTK2B tersambung ke **UART2 ZED-F9P**. Ini berarti:

- **RTCM3 mengalir di hardware**, tidak pernah melewati Arduino
- **Rover tidak butuh mikrokontroler** — ZED-F9P menerima RTCM3 dan mengirim UBX lewat UART2 yang sama
- **Arduino Base cukup satu `SoftwareSerial` receive-only** — inilah yang membuat Arduino UNO memadai

```
BASE                                              ROVER
┌────────────────────────────┐                    ┌──────────────────────────┐
│ ZED-F9P UART2 TX ──────────┼──> XBee DIN        │  XBee DOUT ──> UART2 RX  │
│   RTCM3 1005/1074/1094     │      │             │    (koreksi masuk)       │
│                            │      │  downlink   │                          │
│                            │   ~~~~~~~~~~~~~~~~~~~~~~~~                    │
│                            │      │  uplink     │                          │
│ XBee DOUT ──> pin 2 (RX)   │      │             │  UART2 TX ──> XBee DIN   │
│   Arduino UNO              │      │             │    UBX NAV-PVT +         │
│      │                     │                    │        NAV-RELPOSNED     │
│      └─> USB @115200 ──────┼──> Dashboard       └──────────────────────────┘
└────────────────────────────┘
```

**Base GNSS tidak disambung ke Arduino.** Posisi Base sudah konstan dari TMODE3 Fixed Mode dan ditulis sebagai konstanta di firmware.

### Mode Operasi

| Mode | Baud Dashboard | Format | Di Map | Kebutuhan |
|---|---|---|---|---|
| **NMEA (uji cepat)** | 38400 | NMEA `$GNRMC`/`$GNGGA` | 1 titik (Rover) | Rover GNSS + XBee + Arduino passthrough (`default.ino`) |
| **JSON (sistem penuh)** | **115200** | `{"base":{...},"rover":{...}}` | 2 titik + baseline | Base + Rover GNSS + 2 XBee + Arduino UNO |

### Tidak Ada Data IMU

Karena Rover tanpa MCU, `pitch` dan `roll` selalu `0.0`. Kalau IMU nanti dibutuhkan, lihat catatan Fase 8 di `Arduino_Rover/Arduino_Rover.ino` — ada modifikasi hardware yang wajib dilakukan lebih dulu.

---

## 2. Hardware & Wiring

### Komponen

| Qty | Komponen | Catatan |
|---|---|---|
| 2x | simpleRTK2B (ZED-F9P) | 1 Base + 1 Rover |
| 2x | XBee SX 868/915 MHz | Terpasang di socket onboard simpleRTK2B |
| 2x | Antena GNSS | Base di tempat statis, clear sky |
| 1x | Arduino UNO | **Hanya di Base** |
| 1x | Power bank / baterai 5V | Rover |
| — | Kabel jumper female-female | Base: XBee DOUT → pin 2 |

### Wiring Rover — Tanpa Kabel Tambahan

XBee terpasang di socket, ZED-F9P dan XBee saling terhubung lewat UART2 di PCB. Yang perlu:

```
Antena GNSS  ──> konektor SMA simpleRTK2B
Power 5V     ──> simpleRTK2B (shield menyediakan regulator 3.3V untuk XBee)
```

> ⚠️ Jangan memberi 5V langsung ke XBee. Selalu lewat shield simpleRTK2B.

### Wiring Base — Satu Jumper

| Dari | Ke | Fungsi |
|---|---|---|
| XBee **DOUT** | Arduino UNO **pin 2** | Uplink UBX dari Rover |
| Arduino **GND** | simpleRTK2B **GND** | Referensi ground bersama |
| Arduino **USB** | PC | JSON ke dashboard |

Pin 3 dideklarasikan sebagai TX di firmware tetapi **tidak disambung** — `SoftwareSerial` mensyaratkan argumen TX meski jalur ini receive-only.

> ⚠️ Ground bersama wajib. Tanpa referensi ground yang sama, level sinyal serial tidak terdefinisi dan data akan tampak acak.

Tidak ada jumper ke pin 4, 5, atau 8. Base GNSS **tidak** disambung ke Arduino.

---

## 3. Konfigurasi u-center — Base

Download u-center: https://www.u-blox.com/en/product/u-center

Base dikonfigurasi **dua tahap**: Survey-In untuk mengukur posisi, lalu dikunci ke Fixed Mode.

### 3a. Tahap A — Survey-In

Pasang antena Base di **posisi final** (tidak akan dipindah), clear sky.

| Menu | Setting | Nilai |
|---|---|---|
| **View → Configuration View → TMODE3** | Mode | **1 - Survey-In** |
| | Minimum Observation Time | **300** detik |
| | Required Position Accuracy | **2.0** m |
| **Receiver → Action** | Save Config | ⚠️ Wajib |

Pantau **View → Messages View → UBX-NAV-SVIN** sampai:
- `valid = 1`
- `active = 0`

Ini menandakan survei selesai. Perkiraan 5-10 menit.

### 3b. Tahap B — Kunci ke Fixed Mode

Dari `UBX-NAV-SVIN`, catat **enam** nilai:

| Field | Satuan |
|---|---|
| `meanX`, `meanY`, `meanZ` | cm (ECEF) |
| `meanXHP`, `meanYHP`, `meanZHP` | 0.1 mm (komponen presisi tinggi) |

> ⚠️ Komponen `HP` mudah terlewat. Tanpanya, presisi posisi Base turun dari sub-milimeter ke sentimeter — dan seluruh pengukuran Rover ikut bergeser secara sistematis.

Masukkan ke TMODE3:

| Menu | Setting | Nilai |
|---|---|---|
| **TMODE3** | Mode | **2 - Fixed Mode** |
| | Coordinate system | **ECEF** |
| | X, Y, Z | dari `meanX/Y/Z` |
| | X HP, Y HP, Z HP | dari `meanXHP/YHP/ZHP` |
| **Receiver → Action** | Save Config | ⚠️ Wajib |

Setelah ini Base **boot instan** tanpa menunggu survei, dan posisi absolutnya identik di setiap sesi.

Catat juga **lat/lon/alt** hasil Fixed Mode — nilai ini masuk ke firmware (§6).

### 3c. Output RTCM3 di UART2

| Menu | Setting | Nilai |
|---|---|---|
| **PRT (Ports)** | Target | **2 - UART2** |
| | Protocol in | **RTCM3** |
| | Protocol out | **RTCM3** |
| | Baudrate | **19200** |
| **MSG (Messages)** | RTCM3 **1005** → UART2 | Rate **10** (= 0.1 Hz) |
| | RTCM3 **1074** → UART2 | Rate **1** (GPS MSM4) |
| | RTCM3 **1094** → UART2 | Rate **1** (Galileo MSM4) |
| **Receiver → Action** | Save Config | ⚠️ Wajib |

Alasan pilihan ini:

| Keputusan | Alasan |
|---|---|
| MSM4, bukan MSM7 | MSM7 membawa presisi ekstra yang tidak terpakai di baseline panjang, dengan biaya bandwidth besar |
| 1005 @0.1 Hz | Posisi Base konstan — mengirim 1 Hz adalah pemborosan |
| Tanpa GLONASS/BeiDou (1084/1124/1230) | Penghematan bandwidth terbesar. GPS + Galileo cukup untuk RTK. |

Kalau §8 tahap 2 menunjukkan link masih saturasi, urutan penurunan: buang 1094 → turunkan uplink ke 0.5 Hz → 1074 ke 0.5 Hz.

### 3d. Simpan Konfigurasi ke Repo

**Tools → GNSS Configuration → Save to file.** Simpan sebagai `config/base_config.txt`. Kalau board ter-reset, konfigurasi bisa direstore tanpa mengulang survei.

---

## 4. Konfigurasi u-center — Rover

| Menu | Setting | Nilai |
|---|---|---|
| **PRT (Ports)** | Target | **2 - UART2** |
| | Protocol in | **RTCM3** |
| | Protocol out | **UBX** |
| | Baudrate | **19200** |
| **MSG (Messages)** | UBX-NAV-PVT (0x01 0x07) → **UART2** | ✅ Rate **1** |
| | UBX-NAV-RELPOSNED (0x01 0x3C) → **UART2** | ✅ Rate **1** |
| **Receiver → Action** | Save Config | ⚠️ Wajib |

Tiga hal yang mudah keliru:

1. **Enable message di UART2, bukan UART1.** Enable per-port. Kalau di UART1, data tidak akan sampai ke XBee.
2. **Protocol out = UBX saja.** NMEA dimatikan supaya tidak memakan bandwidth yang dibutuhkan UBX.
3. **Baud 19200** — harus sama dengan UART2 Base dan `BD` kedua XBee.

Simpan sebagai `config/rover_config.txt`.

### Baud Rate — Titik Kegagalan Paling Sering

Kelima nilai ini **harus identik**:

| Titik | Nilai |
|---|---|
| UART2 ZED-F9P Base | 19200 |
| XBee SX Base (`BD`) | 19200 |
| XBee SX Rover (`BD`) | 19200 |
| UART2 ZED-F9P Rover | 19200 |
| `RADIO_BAUD` di `Arduino_Base.ino` | 19200 |

USB Serial Base → Dashboard terpisah: **115200**.

---

## 5. XBee SX

> Panduan langkah demi langkah XCTU — dari Discover Devices, Console, Range
> Test, hingga Recovery: **[xbee_xctu_test.md](xbee_xctu_test.md)**.

XBee dari kit ArduSimple umumnya sudah dipasangkan dari pabrik. Yang **wajib** diubah adalah baud dan mode long-range.

### Setting di XCTU

| Parameter | Base | Rover | Keterangan |
|---|---|---|---|
| **BD** (Baud) | **19200** | **19200** | Harus sama dengan UART2 ZED-F9P |
| **RF data rate** | Long range | Long range | Menukar throughput dengan jangkauan |
| **PAN ID** | sama | sama | Harus identik |
| **DH + DL** | MAC Rover | MAC Base | Saling menunjuk |

Catat **RF data rate aktual** setelah diset — angka ini dipakai di §8 tahap 2 untuk memastikan bandwidth cukup.

> ⚠️ **Regulasi**: 868 MHz adalah alokasi Eropa. Indonesia mengalokasikan pita SRD di sekitar 920-923 MHz. Verifikasi varian modul dan aturan Komdigi sebelum deployment lapangan.

---

## 6. Firmware Arduino Base

### Isi Koordinat Base Dulu

Buka `Arduino_Base/Arduino_Base.ino`, isi dari hasil §3b:

```cpp
#define BASE_CONFIGURED 1          // 0 -> 1
#define BASE_LAT_1E7 -61753924L    // lat x 1e7
#define BASE_LON_1E7 1068271532L   // lon x 1e7
#define BASE_ALT_MM  15420L        // alt dalam mm
```

Selama `BASE_CONFIGURED 0`, dashboard berjalan mode Rover-only dan **tidak** menggambar marker Base. Ini disengaja — lebih baik tidak ada titik daripada titik palsu yang tampak sah.

### Upload

1. Arduino IDE → **Tools → Board**: Arduino UNO
2. **Tools → Port**: COM port Arduino Base
3. **Upload**

Tidak perlu mencabut jumper apa pun — pin 2 tidak dipakai USB serial (yang memakai pin 0 dan 1).

### Ringkasan

| Jalur | Baud | Fungsi |
|---|---|---|
| USB (`Serial`) | 115200 | JSON ke dashboard |
| Pin 2 (`SoftwareSerial` RX) | 19200 | UBX dari Rover via XBee |

Struktur program:

```
loop():
  1. Baca byte dari XBee, jalankan state machine UBX
  2. NAV-PVT (0x01 0x07)      -> posisi, fix, hAcc, speed, heading
     NAV-RELPOSNED (0x01 0x3C) -> baseline North/East
  3. sendJson() @1 Hz -> koordinat Base (konstanta) + data Rover
```

Contoh output:
```json
{"base":{"valid":1,"lat":-6.1753924,"lon":106.8271532,"alt":15.420},"rover":{"id":1,"link_ok":1,"lat":-6.1754100,"lon":106.8272100,"alt":15.500,"fix":4,"hAcc_cm":1.2,"speed_kmh":14.50,"heading":128,"pitch":0.0,"roll":0.0,"relN_m":42.15,"relE_m":18.70}}
```

| Field | Arti |
|---|---|
| `base.valid` | `0` = `BASE_CONFIGURED` belum diisi |
| `rover.link_ok` | `0` = tidak ada UBX valid >3 detik |

### Pemakaian Resource

| Sketch | Flash | RAM Global |
|---|---|---|
| `Arduino_Base.ino` | 6.814 B (21%) | 469 B (22%) |

---

## 7. Dashboard

### Browser
**Chrome** atau **Edge** — hanya keduanya yang mendukung Web Serial API. File lokal (`file://`) diizinkan.

### Fitur

| Fitur | Keterangan |
|---|---|
| Baud selector | **38400** mode NMEA / **115200** mode JSON |
| 🔌 Hubungkan ke Serial | Pilih COM port, mulai streaming |
| Auto-follow Rover | Checkbox — matikan untuk inspeksi manual peta |
| 🔴 Record / ⏹️ Stop | Simpan log ke memori browser |
| 📥 CSV / 🗺️ GPX / 📍 KML | Ekspor; CSV mencakup `hAcc_cm`, `pitch`, `roll` |
| 📌 Base marker | Disembunyikan otomatis kalau `base.valid = 0` |
| 🚤 Rover marker + trail | Posisi dan lintasan Rover |
| Baseline line | Garis Base→Rover, auto-switch ke km untuk jarak jauh |
| **LINK LOST** | Badge + marker diredupkan kalau paket >3 detik tidak masuk |
| Fix Status | RTK FIX / RTK FLOAT / DGPS / SINGLE / NO FIX |

> Label fix status tidak mencantumkan angka akurasi karena akurasi bergantung panjang baseline. Rujuk `hAcc` untuk nilai aktual.

Tampilan awal memakai `fitBounds` agar Base dan Rover keduanya terlihat meski berjarak beberapa km.

### Cara Pakai — Mode JSON

1. Pastikan §3, §4, §5, §6 selesai
2. Nyalakan Base GNSS + Rover GNSS (keduanya dengan XBee)
3. Buka `map_dashboard.html` di Chrome
4. Pilih baud **115200**
5. **🔌 Hubungkan ke Serial** → pilih COM port Arduino Base
6. Muncul 2 titik dengan baseline line

Base tidak perlu waktu tunggu — Fixed Mode membuatnya siap sejak boot. Rover butuh 1-2 menit untuk cold start dan lock RTK.

### Cara Pakai — Mode NMEA (uji cepat Rover)

1. Upload `default.ino` ke Arduino sebagai passthrough
2. Sambung XBee ke pin 0 (RX), 1 (TX)
3. Baud **38400**, connect

---

## 8. Bring-Up Berlapis

Uji berlapis. Setiap tahap mengisolasi satu kemungkinan penyebab kegagalan.

| Tahap | Uji | Kriteria Lolos |
|---|---|---|
| **1** | Link XBee terisolasi (`xbee_test.md`, baud **19200**) | LED Rover berkedip tiap detik |
| **2** | Ukur throughput: UART2 Base → PC, hitung B/s | Downlink + uplink < kapasitas RF |
| **3** | **Baseline ~50 m** | **RTK FIX tercapai** |
| **4** | 500 m | RTK FIX bertahan |
| **5** | 2 km | FIX atau FLOAT stabil |
| **6** | 5 km → target | FLOAT stabil, packet loss terukur |

**Tahap 3 adalah gerbang terpenting.** Kalau di 50 m tidak dapat RTK FIX, masalahnya **konfigurasi u-center — bukan jangkauan radio**. Menaikkan jarak sebelum tahap ini lolos hanya menambah variabel yang harus didebug.

Catat di setiap tahap: `fix` status, `hAcc` aktual, packet loss. Data ini sekaligus menjadi hasil range test yang dapat dilaporkan.

### Ekspektasi Akurasi

| Baseline | Realistis |
|---|---|
| < 1 km | RTK FIX, 1-2 cm |
| 1-5 km | RTK FIX dominan, sesekali FLOAT |
| **5-10 km** | **RTK FLOAT, 10-50 cm** |
| > 10 km | Tidak stabil |

Pada baseline >5 km, **jangan mengklaim akurasi 1-2 cm** — dekorelasi atmosfer membuat integer ambiguity sulit terkunci. Lihat `README.md` §4B dan `technical_terms.md` §4.

---

## 9. Troubleshooting

### Dashboard tidak connect

| Masalah | Solusi |
|---|---|
| `requestPort` gagal | Hanya Chrome/Edge. Pastikan port tidak dibuka Arduino IDE Serial Monitor. |
| Port terdaftar tapi tidak ada data | Cek baud dashboard: 115200 untuk JSON, 38400 untuk NMEA |
| Port tidak muncul | Cabut-pasang USB. Cek driver CH340 di Device Manager. |

### Marker Base tidak muncul

| Masalah | Solusi |
|---|---|
| Panel Base hilang, subtitle "Rover Tracker" | `BASE_CONFIGURED` masih `0`. Selesaikan §3b, isi koordinat, upload ulang. |

### Rover tidak muncul / LINK LOST

| Masalah | Solusi |
|---|---|
| LINK LOST terus | Jalankan §8 tahap 1. Kalau XBee OK, cek NAV-PVT/RELPOSNED di-enable pada **UART2** Rover (bukan UART1). |
| Data masuk tapi acak | **Baud mismatch.** Verifikasi kelima titik di tabel §4. Cek juga GND bersama Arduino-simpleRTK2B. |
| LINK LOST intermiten | Packet loss. Turunkan rate sesuai §3c, tinggikan antena, pastikan line-of-sight. |

### Tidak dapat RTK FIX

| Masalah | Solusi |
|---|---|
| Tetap SINGLE (`fix=1`) | RTCM3 tidak sampai. Cek §3c — Protocol out UART2 Base harus **RTCM3**, dan 1074/1094 di-enable. |
| SINGLE di baseline 50 m | Masalah konfigurasi. Verifikasi Base sudah Fixed Mode (§3b), bukan masih Survey-In. |
| FLOAT tidak naik ke FIX di 50 m | Cek `hAcc`. Kalau >1 m, kemungkinan multipath — pindahkan antena. |
| FLOAT di >5 km | **Normal.** Lihat ekspektasi akurasi §8. |
| NO FIX | Antena di luar ruangan, clear sky. Tunggu 1-2 menit cold start. |

### Hardware

| Masalah | Solusi |
|---|---|
| simpleRTK2B tidak menyala | Power 5V dengan arus >200 mA. LED merah harus menyala. |
| XBee tidak menyala | Cek pemasangan di socket. Jangan beri 5V langsung. |

---

## Ringkasan Cepat

```
Base:   ZED-F9P UART2 --RTCM3--> XBee ~~~> XBee --RTCM3--> ZED-F9P UART2 :Rover
Rover:  ZED-F9P UART2 --UBX----> XBee ~~~> XBee --> pin 2 Arduino --> USB --> Dashboard
```

| | Mode NMEA | Mode JSON |
|---|---|---|
| **Baud dashboard** | 38400 | 115200 |
| **Firmware Base** | `default.ino` passthrough | `Arduino_Base.ino` |
| **Firmware Rover** | Tidak ada | Tidak ada |
| **u-center Base** | — | TMODE3 Fixed + RTCM3 UART2 |
| **u-center Rover** | NMEA UART1 | UBX UART2, NAV-PVT + RELPOSNED |
| **Map** | 1 titik | 2 titik + baseline |
| **Akurasi** | ~1.5 m | 1-2 cm (dekat) → 10-50 cm (>5 km) |

---

*Coastal Driffter Setup Guide — revisi 2026.*
