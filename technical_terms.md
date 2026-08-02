# Coastal Driffter — Glosarium & Referensi Teknis

Dokumen referensi akademik untuk memahami setiap istilah teknis dalam sistem Coastal Driffter, dari GNSS fundamental hingga korelasinya dalam dashboard real-time.

---

## Daftar Isi

1. [GNSS Fundamentals](#1-gnss-fundamentals)
2. [NMEA 0183 Protocol](#2-nmea-0183-protocol)
3. [UBX Protocol](#3-ubx-protocol)
4. [RTK Theory](#4-rtk-theory)
5. [RTCM 3.x Correction Data](#5-rtcm-3x-correction-data)
6. [Fix Status & Quality Metrics](#6-fix-status--quality-metrics)
7. [Base Station & Survey-In](#7-base-station--survey-in)
8. [RELPOSNED — Relative Positioning](#8-relposned--relative-positioning)
9. [Dilution of Precision (DOP)](#9-dilution-of-precision-dop)
10. [Serial Communication](#10-serial-communication)
11. [XBee & ISM Radio](#11-xbee--ism-radio)
12. [Sistem Koordinat & Geometri Bola](#12-sistem-koordinat--geometri-bola)
13. [Korelasi Penuh di Coastal Driffter](#13-korelasi-penuh-di-coastal-driffter)

---

## 1. GNSS Fundamentals

### 1.1 Sejarah dan Arsitektur GNSS

GNSS (Global Navigation Satellite System) adalah sistem penentuan posisi berbasis konstelasi satelit di orbit Bumi. Perjalanan teknologinya dimulai dari sistem TRANSIT milik US Navy pada 1960-an — hanya 5-7 satelit di orbit polar rendah (~1100 km) dengan akurasi ±200 meter. Era modern dimulai dengan peluncuran satelit GPS pertama pada 1978, disusul deklarasi operasional penuh GPS pada 1995 oleh US Department of Defense dengan 24 satelit di 6 orbital plane.

GLONASS (Rusia) mengikuti jejak GPS dengan peluncuran pertamanya pada 1982, mencapai operasional penuh 24 satelit pada 1996 — meskipun sempat mengalami degradasi hingga hanya 8 satelit operasional pasca keruntuhan Soviet, kemudian direstorasi penuh pada 2011. GLONASS unik karena menggunakan FDMA (Frequency Division Multiple Access): setiap satelit mentransmisikan pada frekuensi L1 yang sedikit berbeda (L1 = 1602MHz + k×0.5625MHz untuk satelit k, dengan k = −7 sampai +6).

Galileo (Uni Eropa) adalah sistem sipil pertama — tidak dikendalikan militer — diluncurkan bertahap sejak 2011 dan mencapai operasional penuh dengan 24 satelit + 6 spare pada 2020-an. Galileo mengklaim akurasi open service hingga 20cm dengan sinyal E1/E5/E6 menggunakan modulasi AltBOC.

BeiDou (China) dimulai sebagai sistem regional BeiDou-1 (2000), berekspansi ke regional BeiDou-2 (2012, 14 satelit untuk Asia-Pasifik), dan mencapai operasional global BeiDou-3 pada 2020 dengan 30 satelit: 24 MEO (Medium Earth Orbit) + 3 GEO (Geostationary) + 3 IGSO (Inclined Geo-Synchronous Orbit) — konstelasi hybrid unik yang mengoptimalkan coverage di Asia. Satelit GEO BeiDou menyediakan SBAS-like augmentation dan komunikasi dua arah.

Sistem GNSS dibagi menjadi tiga segmen fundamental:

1. **Space Segment**: konstelasi satelit di orbit MEO (~20.200 km untuk GPS/Galileo/BeiDou, ~19.100 km untuk GLONASS). Setiap satelit membawa jam atom (cesium 2-3 unit, rubidium untuk GLONASS) dengan stabilitas ~10⁻¹³ per hari — error clock 1 nanodetik = 30cm error posisi. Satelit menyiarkan dua hal melalui sinyal RF L-band (1-2 GHz): (a) ranging code (C/A code sipil pada L1 1575.42MHz — chip rate 1.023MHz, panjang 1023 chip, periode 1ms; P(Y) code militer pada L1/L2 — chip rate 10.23MHz, panjang ~38 minggu; L2C dan L5 untuk sipil modern) dan (b) navigation message (ephemeris — parameter orbit presisi untuk satelit tersebut, berlaku 2-4 jam; almanac — orbit kasar untuk semua satelit, berlaku hingga 90 hari; ionospheric parameters; satellite health status).

2. **Control Segment**: jaringan ground station global yang melacak satelit, menghitung ephemeris, dan meng-upload parameter koreksi. GPS memiliki Master Control Station di Colorado Springs, 16 monitoring stations, dan 4 ground antennas. Galileo memiliki GCC (Galileo Control Centre) di Oberpfaffenhofen (Jerman) dan Fucino (Italia).

3. **User Segment**: receiver seperti u-blox ZED-F9P yang digunakan dalam Coastal Driffter — menerima sinyal L-band, melakukan akuisisi dan tracking, mendekode navigation message, menghitung pseudorange dan carrier phase, lalu menyelesaikan persamaan navigasi untuk posisi, kecepatan, dan waktu (PVT solution).

**Multi-constellation GNSS** adalah kemampuan receiver untuk melacak satelit dari beberapa konstelasi secara simultan. Pada ZED-F9P, receiver dapat melacak GPS L1C/A + L2C, GLONASS L1OF + L2OF, Galileo E1-B/C + E5b, dan BeiDou B1I + B2I secara bersamaan. Hasilnya: lebih banyak satelit tersedia (bisa >40 satelit di langit terbuka), geometri yang lebih baik (DOP lebih rendah), dan waktu menuju fix pertama (TTFF — Time To First Fix) yang lebih cepat dari cold start. Di dashboard Coastal Driffter, sentence NMEA menggunakan talker ID `GN` (seperti `$GNRMC`, `$GNGGA`) yang mengindikasikan posisi dihitung dari kombinasi multi-konstelasi — bukan hanya GPS (yang menggunakan talker `GP`).

### 1.2 Prinsip Trilaterasi

GNSS menentukan posisi melalui trilaterasi, bukan triangulasi. Triangulasi menggunakan pengukuran sudut dari dua titik referensi yang diketahui; trilaterasi menggunakan pengukuran jarak dari titik referensi (satelit) yang posisinya diketahui.

Setiap satelit menyiarkan sinyal yang mengandung: posisi satelit saat transmisi (dari ephemeris), dan waktu transmisi presisi (dari jam atom onboard). Receiver mencatat waktu kedatangan sinyal menggunakan jam quartz internal. Pseudorange (ρ, "pseudo" karena mengandung error clock receiver) dinyatakan sebagai:

```
ρ = c × (t_receive − t_transmit)
```

di mana `c = 299.792.458 m/s` adalah kecepatan cahaya dalam vakum. Dengan satu satelit, receiver tahu bahwa posisinya terletak pada permukaan bola (sphere) dengan radius ρ berpusat di satelit tersebut. Dua satelit membentuk irisan dua bola: sebuah lingkaran. Tiga satelit mempersempit menjadi dua titik: satu di permukaan Bumi, satu di luar angkasa. Tetapi ada komplikasi: **receiver clock error**.

Receiver menggunakan jam quartz crystal (ATC-310 atau setara pada ZED-F9P) — murah dan kecil, tetapi tidak seakurat jam atom. Error clock 1 mikrodetik — tipikal clock drift consumer — menghasilkan error jarak 300 meter. Karena itu diperlukan **satelit keempat** untuk menyelesaikan empat variabel yang tidak diketahui: posisi 3D (latitude, longitude, altitude) + receiver clock bias.

Persamaan pseudorange untuk satelit ke-i:

```
ρᵢ = √[(xᵢ − x)² + (yᵢ − y)² + (zᵢ − z)²] + c·(dt − dTᵢ) + Iᵢ + Tᵢ + εᵢ
```

di mana:
- `(x, y, z)` = posisi receiver (tidak diketahui)
- `(xᵢ, yᵢ, zᵢ)` = posisi satelit ke-i (dari ephemeris)
- `dt` = receiver clock offset (tidak diketahui)
- `dTᵢ` = satellite clock offset (dari navigation message)
- `Iᵢ` = ionospheric delay (frekuensi-dependent, ~1-50m)
- `Tᵢ` = tropospheric delay (frekuensi-independent, ~2-25m)
- `εᵢ` = multipath + noise + residual errors

Untuk menyelesaikan 4 variabel tidak diketahui `(x, y, z, dt)`, dibutuhkan minimal 4 persamaan pseudorange. Sistem ini non-linear sehingga diselesaikan secara iterative menggunakan linearisasi Newton-Raphson:

```
Δx = (HᵀH)⁻¹ Hᵀ Δρ
```

di mana H adalah matriks desain (design matrix) berdimensi N×4 yang berisi directional cosine derivatives terhadap posisi dan clock. Solusi di-update secara iteratif hingga konvergen (biasanya 3-5 iterasi). Matriks `(HᵀH)⁻¹` inilah yang menjadi dasar perhitungan DOP (Dilution of Precision).

### 1.3 Sumber Error pada Pengukuran GNSS

**Satellite Clock Error (~2m, corrected)**: Jam atom onboard satelit — meskipun sangat stabil — mengalami drift karena efek relativistik (general + special relativity). Satelit GPS bergerak pada ~3.9 km/s relatif terhadap receiver di permukaan Bumi — special relativity (time dilation) memperlambat jam satelit sebesar ~7 μs/hari, sementara general relativity (gravitational blueshift) mempercepat jam sebesar ~45 μs/hari. Net effect adalah jam satelit lebih cepat ~38 μs/hari dibandingkan jam di permukaan Bumi. Tanpa koreksi ini, error posisi 24 jam = 38 μs × 3×10⁸ m/s = 11.4 km! Koreksi relativistik sudah diterapkan di hardware satelit (frekuensi fundamental 10.23MHz dikurangi menjadi 10.22999999543MHz pre-launch) dan residual error dikoreksi melalui parameter broadcast dalam navigation message.

**Ephemeris Error (~2.5m)**: Posisi satelit tidak diketahui secara sempurna. Prediksi orbit dari Control Segment memiliki akurasi ~1-2m (RMS). Ephemeris berlaku 2-4 jam — setelah itu akurasi orbit menurun drastis. GLONASS memiliki akurasi ephemeris yang sedikit lebih rendah dibanding GPS (~3-5m) karena jumlah monitoring station yang lebih sedikit.

**Ionospheric Delay (1-50m, worst case saat solar maximum)**: Lapisan ionosfer (50-1000 km) mengandung elektron bebas yang memperlambat gelombang elektromagnetik — bersifat **dispersif** (frekuensi-dependent). Semakin rendah frekuensi, semakin besar delay. Pada frekuensi L1 (1575.42 MHz), delay ionosfer vertical bisa mencapai 30m di ekuator saat aktivitas matahari maksimum. Koreksi ionosfer single-frequency: menggunakan model Klobuchar (8 parameter broadcast dalam navigation message) — akurasi ~50% dari total delay, masih menyisakan error ~2-5m. Koreksi dual-frequency (yang dilakukan ZED-F9P secara otomatis dengan tracking L1+L2): karena ionosfer bersifat dispersif, perbandingan delay di L1 vs L2 memungkinkan eliminasi efek orde pertama secara hampir sempurna — error residual <0.1m. Inilah mengapa **RTK memerlukan receiver dual-frequency** — tanpa koreksi ionosfer akurat, integer ambiguity tidak bisa di-resolve secara reliable.

**Tropospheric Delay (2-25m, zenith)**: Lapisan troposfer (0-50 km) mengandung gas netral (N₂, O₂, H₂O) yang membiaskan sinyal. Tidak seperti ionosfer, troposfer bersifat **non-dispersif** di frekuensi GNSS — semua frekuensi ter-delay sama besar → tidak bisa dihilangkan dengan dual-frequency. Delay troposfer dibagi menjadi komponen kering (dry/hydrostatic, ~2.3m zenith, 90% total, diprediksi dari surface pressure) dan komponen basah (wet, ~0-30cm zenith, 10% total, diprediksi dari water vapor — jauh lebih tidak pasti). Model: Saastamoinen, Hopfield. Error residual troposfer ~10-30cm di zenith, meningkat 5-10x pada elevation rendah. Di Coastal Driffter, ini adalah salah satu sumber error dominan untuk baseline panjang (>10 km).

**Multipath (0.1-5m)**: Sinyal yang mencapai receiver melalui lebih dari satu jalur — pantulan dari permukaan tanah, bangunan, air, atau objek metalik di dekat antena. Sinyal pantulan selalu tiba lebih lambat dari direct path (geometri segitiga) dan memiliki polarisasi yang berubah — interferensi destruktif menghasilkan osilasi pseudorange error. Di lingkungan laut (Coastal Driffter), multipath dari permukaan air sangat signifikan karena air adalah reflektor superior (konstanta dielektrik tinggi ~80 pada L-band). Antena GNSS dengan choke ring atau ground plane membantu menekan multipath low-elevation, tetapi tidak bisa menghilangkannya sepenuhnya. Ciri multipath di data: error berosilasi dengan perioda 5-20 menit (sebanding dengan perubahan geometri satelit).

**Receiver Noise (~0.1-0.3m)**: Thermal noise di front-end RF (LNA, mixer, ADC). Tergantung desain receiver, bandwidth, dan signal processing gain. ZED-F9P memiliki noise floor ~0.2m untuk C/A code pseudorange dan ~1mm untuk carrier phase.

**Total Error (Root-Sum-Square)**: Error-error di atas tidak berkorelasi penuh sehingga RSS (root of sum of squares) lebih tepat daripada penjumlahan langsung:

```
σ_total = √(σ²_clock + σ²_ephemeris + σ²_iono + σ²_tropo + σ²_mp + σ²_noise)
```

Untuk GPS single-point C/A code: σ_total ≈ √(4 + 6.25 + 9 + 6.25 + 1 + 0.09) ≈ 5.2m (RMS horizontal, 1-sigma). Tipikal 95% (2-drms) = ~10m. Ini sesuai dengan pengalaman lapangan untuk mode NMEA (SINGLE fix) di Coastal Driffter.

### 1.4 Frekuensi dan Sinyal — Mengapa Multi-Band Penting

Spektrum L-band (1-2 GHz) dipilih untuk GNSS karena merupakan sweet spot antara: (a) cukup rendah untuk penetrasi atmosfer dengan atenuasi minimal, (b) cukup tinggi untuk antena compact (λ/4 patch antenna ~5cm di L1), dan (c) bandwidth cukup lebar untuk chip rate tinggi.

Frekuensi GPS:
- **L1** (1575.42 MHz, λ=19.03 cm): C/A code (sipil, 1.023 Mcps, chipping rate), P(Y) code (militer/terenkripsi, 10.23 Mcps), L1C (sipil modern, BOC(1,1) modulation)
- **L2** (1227.60 MHz, λ=24.42 cm): P(Y) code, L2C (sipil, CM+CL code, lebih sensitif untuk tracking lemah)
- **L5** (1176.45 MHz, λ=25.48 cm): safety-of-life, higher power, untuk aviation — ZED-F9P tidak menggunakan L5 (hanya L1+L2)

Frekuensi GLONASS (FDMA — setiap satelit berbeda):
- **L1** = 1602 + k×0.5625 MHz (λ ≈ 18.7 cm)
- **L2** = 1246 + k×0.4375 MHz (λ ≈ 24.0 cm)
- k = frequency channel number (−7 to +6, anti-polar satellites share same k)

Galileo:
- **E1** (1575.42 MHz — overlap dengan GPS L1 untuk interoperabilitas)
- **E5a** (1176.45 MHz), **E5b** (1207.14 MHz), AltBOC E5 (E5a+E5b, bandwidth 51.15 MHz)

BeiDou:
- **B1I** (1561.098 MHz) — ZED-F9P tracks B1I

**Mengapa dual-band (L1+L2) kritis untuk RTK**:

Efek ionosfer bersifat dispersif — delay group berbeda untuk setiap frekuensi. Dengan mengukur pseudorange di dua frekuensi (ρL1 dan ρL2), kita dapat membentuk **iono-free combination**:

```
ρ_IF = (f²L1 × ρL1 − f²L2 × ρL2) / (f²L1 − f²L2)
```

Kombinasi ini menghilangkan efek ionosfer orde pertama (>99.9%). Efek orde kedua (~1-2cm) masih tersisa — signifikan hanya untuk geodesi presisi. Dengan ionosfer yang dieliminasi, integer ambiguity bisa di-resolve lebih cepat dan lebih reliable. Inilah mengapa receiver single-frequency (L1 only) tidak bisa mencapai RTK FIX dengan baseline >1-2 km secara reliable.

**Carrier phase measurement** — fondasi RTK — menggunakan gelombang pembawa (carrier wave) yang berfrekuensi sangat tinggi (L1 = 1575.42 MHz, λ ≈ 19 cm). Receiver mengukur fase gelombang saat tiba dengan presisi ~1% dari panjang gelombang = ±2 mm. Ini 100-1000x lebih presisi dari pengukuran pseudorange C/A code (chip 293m, tracking ~0.5m). Tetapi carrier phase measurement bersifat **ambiguous**: receiver tahu di mana dalam siklus terakhir (fractional phase, 0-360°), tetapi tidak tahu berapa banyak siklus penuh antara satelit dan antena. Inilah integer ambiguity yang harus di-resolve dalam proses RTK.

---

## 2. NMEA 0183 Protocol

### 2.1 Sejarah dan Arsitektur

NMEA 0183 (National Marine Electronics Association, 1983) adalah standar komunikasi serial antar instrumen navigasi laut — dikembangkan puluhan tahun sebelum GPS sipil tersedia secara luas. Standar ini mendefinisikan format kalimat teks ASCII yang dikirim melalui RS-232 (single talker, multiple listener) atau RS-422 (differential, noise-immune untuk lingkungan kelautan). Setiap kalimat dimulai dengan karakter `$` dan diakhiri dengan carriage return + line feed (`\r\n`). Baud rate historis adalah 4800 bps — didesain untuk display teks dan chart plotter generasi 1980-an. Implementasi modern mendukung 38400 bps untuk receiver yang menghasilkan lebih banyak data (multi-constellation).

Struktur setiap sentence:

```
$TTSSS,field1,field2,...,fieldN*CC\r\n
```

- `$` — start of sentence delimiter
- `TT` — talker identifier (2 karakter):
  - `GP` = GPS only
  - `GL` = GLONASS only
  - `GA` = Galileo only
  - `GB` = BeiDou only
  - `GN` = multi-constellation combined solution (yang kita lihat di Coastal Driffter)
- `SSS` — sentence formatter (3 karakter): `RMC`, `GGA`, `GLL`, `GSA`, `GSV`, `VTG`, `GST`, dll
- `fields` — comma-separated values, field kosong diindikasikan dengan koma berturutan (`,,`)
- `*` — checksum delimiter
- `CC` — XOR checksum dari semua byte antara `$` dan `*` (tidak termasuk `$` dan `*`), hex uppercase

Checksum NMEA dihitung dengan XOR berantai: `CK = byte1 XOR byte2 XOR ... XOR byteN`. Contoh: untuk sentence `$GNGGA,123519.00,0746.105,S,...` — XOR semua byte dari `G` sampai field terakhir sebelum `*`. Ini adalah checksum sederhana untuk deteksi error bit tunggal — tidak mendeteksi burst error atau byte swap.

### 2.2 $GNRMC — Recommended Minimum Navigation Data

Sentence NMEA yang paling fundamental — membawa informasi minimum yang diperlukan untuk navigasi:

```
$GNRMC,081205.00,A,0746.1055180,S,11023.6272104,E,0.008,128.4,020826,,,A,V*0B
```

| Field | Nama | Contoh | Penjelasan |
|-------|------|--------|------------|
| 1 | UTC Time | `081205.00` | HHMMSS.SS — 08:12:05.00 UTC. Waktu GPS tidak sama dengan UTC (ada offset ~18 leap seconds) |
| 2 | Status | `A` | A = Active (data valid), V = Void (invalid, receiver warning) |
| 3 | Latitude | `0746.1055180` | Format: DDMM.MMMMMM — 07°46.1055180′ |
| 4 | N/S | `S` | Hemisphere — S = South → latitude negatif |
| 5 | Longitude | `11023.6272104` | Format: DDDMM.MMMMMM — 110°23.6272104′ |
| 6 | E/W | `E` | Hemisphere — E = East → longitude positif |
| 7 | Speed (knots) | `0.008` | Speed Over Ground dalam knots (nautical miles/hour) |
| 8 | Track angle | `128.4` | Course Over Ground / true heading dalam derajat (0-360), 0 = utara, 90 = timur |
| 9 | Date | `020826` | DDMMYY — 2 Agustus 2026 |
| 10 | Magnetic var | (kosong) | Magnetic variation (declination) |
| 11 | Var dir | (kosong) | E/W |
| 12 | Mode | `A` | A=autonomous, D=differential, E=estimated, N=data not valid |

**Konversi koordinat NMEA → decimal degrees:**

Format NMEA untuk latitude adalah `DDMM.MMMMMM`. Ini bukan desimal sederhana — perlu dipecah:

```
0746.1055180,S
→ Degrees = int(0746.1055180 / 100) = 07
→ Minutes = 0746.1055180 MOD 100 = 46.1055180
→ Decimal = Degrees + Minutes/60 = 7 + 46.1055180/60 = 7 + 0.76842530 = 7.76842530
→ Hemisphere S → negatif
→ Hasil akhir: -7.76842530°
```

Untuk longitude:

```
11023.6272104,E
→ Degrees = 11023.6272104 / 100 = 110 (integer division)
→ Minutes = 11023.6272104 MOD 100 = 23.6272104
→ Decimal = 110 + 23.6272104/60 = 110 + 0.39378684 = 110.39378684°
→ Hemisphere E → positif
→ Hasil akhir: 110.39378684°
```

Konversi kecepatan: `speed_kmh = speed_knots × 1.852`. 1 knot = 1 nautical mile/jam = 1852 m/jam = 0.5144 m/s. Contoh: 0.008 knots = 0.0148 km/h — receiver stationary di meja.

**Mengapa RMC penting di Coastal Driffter**: RMC adalah sentence NMEA yang membawa data dinamis (speed + heading) yang tidak ada di GGA. Dashboard kita menggunakan RMC untuk melacak pergerakan Rover — tanpa RMC, Speed dan Heading akan selalu 0 atau tidak terisi.

### 2.3 $GNGGA — Fix Data dengan Quality Indicator

Sentence `$GNGGA` adalah yang **paling kritis** untuk menentukan kualitas fix:

```
$GNGGA,081205.00,0746.1055180,S,11023.6272104,E,4,12,1.2,15.420,M,21.3,M,2.0,0000*XX
```

| Field | Nama | Contoh | Penjelasan |
|-------|------|--------|------------|
| 1 | UTC Time | `081205.00` | Sama dengan RMC |
| 2-3 | Latitude | `0746.1055180,S` | Format identik RMC |
| 4-5 | Longitude | `11023.6272104,E` | Format identik RMC |
| 6 | **Fix Quality** | `4` | **Ini yang menentukan badge di dashboard** |
| 7 | Satellites Used | `12` | Jumlah satelit yang digunakan dalam solusi PVT |
| 8 | **HDOP** | `1.2` | Horizontal Dilution of Precision |
| 9 | Altitude (MSL) | `15.420` | Tinggi di atas mean sea level (meter) |
| 10 | Unit Altitude | `M` | M = meter |
| 11 | Geoid Separation | `21.3` | Perbedaan ellipsoid WGS84 - geoid (meter) |
| 12 | Unit Geoid | `M` | |
| 13 | DGPS Age | `2.0` | Umur koreksi DGPS/RTK (detik). Kosong jika tidak ada koreksi |
| 14 | DGPS Station ID | `0000` | ID stasiun referensi — 0000 jika tidak ada base |

**Fix Quality (Field 6) — referensi utama dashboard:**

| Nilai | Nama | Akurasi Khas | Kondisi Terjadi |
|-------|------|--------------|-----------------|
| 0 | Fix Not Available | — | Indoor, antena tidak terpasang, cold start <30 detik pertama |
| 1 | GNSS Fix / Autonomous | 3-15m | 4+ satelit outdoor, tanpa koreksi eksternal. **Mode NMEA normal** |
| 2 | Differential GNSS / DGPS | 0.5-3m | Koreksi dari SBAS satelit (WAAS/EGNOS) atau DGPS beacon terrestrial |
| 4 | RTK Fixed Ambiguity | 1-2cm + 1ppm | **Integer ambiguity resolved** — akurasi centimeter |
| 5 | RTK Float Ambiguity | 10-50cm | **Carrier phase tracked, integer ambiguity not yet resolved** |
| 6 | Dead Reckoning | — | Receiver menggunakan IMU/sensor internal |

Di dashboard Coastal Driffter, field inilah yang diterjemahkan menjadi badge "SINGLE", "DGPS", "RTK FLOAT", atau "RTK FIX".

**HDOP (Field 8)**: HDOP < 1 menunjukkan geometri satelit ideal; >5 menandakan kondisi buruk. HDOP dikalikan dengan measurement error menghasilkan estimasi akurasi horizontal. Untuk mode NMEA tanpa RTK, kita gak punya hAcc numerik (dalam satuan meter), jadi HDOP adalah satu-satunya proxy kualitas. Dashboard kita mengkonversi HDOP × 100 menjadi nilai hAcc_cm sebagai estimasi kasar.

### 2.4 Sentence NMEA Lainnya

**$GNGLL — Geographic Position (Latitude/Longitude)**

Kalimat paling sederhana: hanya posisi 2D tanpa informasi fix quality, altitude, atau speed. Jarang digunakan sendirian.

```
$GNGLL,0746.1055180,S,11023.6272104,E,081205.00,A,A*XX
```

**$GNGSA — GNSS DOP and Active Satellites**

Menampilkan satelit mana yang digunakan dalam solusi PVT dan nilai DOP:

```
$GNGSA,M,3,09,10,27,37,06,07,,,,,,,1.72,0.77,1.54,4*XX
```

- Field 1: Mode (M=manual, A=automatic — 2D/3D selection)
- Field 2: Fix type (1=no fix, 2=2D, 3=3D)
- Field 3-14: PRN numbers dari satelit yang digunakan (kosong jika <12 satelit)
- Field 15: **PDOP** (Positional Dilution of Precision)
- Field 16: **HDOP** (Horizontal)
- Field 17: **VDOP** (Vertical)
- Field 18: GNSS System ID

**$GPGSV / $GLGSV — Satellites in View**

Masing-masing constellation memiliki sentence GSV sendiri. Satu sentence maksimal 4 satelit — untuk >4 satelit diperlukan multiple sentences:

```
$GPGSV,3,1,09,10,38,351,36,27,37,086,30,09,07,059,22,37,06,288,20*XX
$GPGSV,3,2,09,16,15,165,25,08,33,303,30,26,44,191,31,23,48,061,29*XX
$GPGSV,3,3,09,13,25,303,41,,,,,,,,,*XX
```

- Field 1: Total sentences (3)
- Field 2: Sentence number (1,2,3)
- Field 3: Satellites in view (09)
- Setiap satelit (4 per sentence): PRN, Elevation (°), Azimuth (°), C/N0 (dB-Hz)

Elevasi <5° biasanya ditolak receiver untuk menghindari multipath — ini bisa dikonfigurasi.

### 2.5 Keterbatasan NMEA — Mengapa Kita Butuh UBX

NMEA 0183 adalah protokol warisan dengan banyak keterbatasan yang menjadi motivasi mengapa sistem final Coastal Driffter menggunakan **UBX binary protocol** untuk komunikasi antara GNSS dan Arduino Base:

1. **ASCII overhead**: Satu sentence `$GNRMC` sepanjang ~65 byte hanya membawa 6 field berguna. UBX-NAV-PVT 92 byte membawa >30 field — efisiensi bandwidth >10x.

2. **Tidak ada carrSoln flag**: NMEA field fix quality hanya bisa membedakan single/DGPS/RTK_FIX/RTK_FLOAT dalam GGA — tapi ini berasal dari receiver NMEA output, bukan dari internal engine. UBX NAV-PVT byte [21] bits 6-7 (carrSoln) adalah flag internal receiver yang lebih akurat.

3. **Tidak ada RELPOSNED**: NMEA tidak memiliki sentence untuk posisi relatif (baseline vector terhadap Base Station). UBX-NAV-RELPOSNED memberikan informasi ini — fundamental untuk menghitung seberapa jauh Rover dari Base dalam koordinat lokal (North/East).

4. **hAcc tidak dalam meter**: NMEA hanya memberikan HDOP — angka tanpa dimensi yang perlu dikalikan dengan sigma pengukuran (yang tidak diketahui di NMEA). UBX NAV-PVT memberikan hAcc dalam milimeter dari estimasi statistik internal receiver.

5. **Presisi numerik terbatas**: NMEA lat/lon ditampilkan dalam DDMM.MMMMMM — resolusi ~1.85cm di ekuator. UBX menggunakan int32 dengan skala 1e-7 derajat — resolusi ~1.1cm. Untuk RTK dengan akurasi 1-2cm, resolusi NMEA berada di batas bawah.

---

## 3. UBX Protocol

### 3.1 Arsitektur Binary u-blox

UBX adalah protokol binary proprietary u-blox, didesain untuk komunikasi efisien dengan receiver mereka (termasuk seri ZED-F9P). Tidak seperti NMEA yang merupakan standar terbuka lintas vendor, UBX adalah u-blox-specific — keunggulannya adalah akses ke **semua data internal receiver** dengan presisi penuh.

Setiap frame UBX mengikuti struktur:

```
SYNC1  SYNC2  CLASS  ID  LENGTH  PAYLOAD  CK_A  CK_B
 0xB5   0x62   1B    1B   2B LE   N bytes   1B    1B
```

- **SYNC1 (0xB5), SYNC2 (0x62)**: Dua byte synchronisation — pola `µb` (micro-blox) dalam ASCII. Unik: tidak ada kalimat NMEA yang dimulai dengan karakter non-ASCII, sehingga receiver dapat membedakan aliran NMEA vs UBX.
- **CLASS (1 byte)**: Kategori pesan:
  - `0x01` = NAV (Navigation — posisi, kecepatan, waktu)
  - `0x02` = RXM (Receiver Manager — raw data)
  - `0x04` = INF (Information — debug strings)
  - `0x05` = ACK (Acknowledge — ACK/NAK)
  - `0x06` = CFG (Configuration — setting receiver)
  - `0x0A` = MON (Monitoring — status, msgs processed)
  - `0x0D` = TIM (Timing)
- **ID (1 byte)**: Sub-tipe pesan dalam class. Contoh: NAV-PVT = 0x07 dalam class NAV (0x01).
- **LENGTH (2 byte, little-endian)**: Panjang payload, max 65535 byte (praktis terbatas oleh buffer receiver).
- **PAYLOAD (N bytes)**: Data terstruktur — setiap message memiliki layout field yang didefinisikan dalam dokumentasi u-blox.
- **CK_A, CK_B (masing-masing 1 byte)**: Fletcher checksum — bukan XOR sederhana.

**Fletcher Checksum Algorithm:**

```
CK_A = 0, CK_B = 0
For each byte in (CLASS + ID + LENGTH_bytes + PAYLOAD):
    CK_A = (CK_A + byte) % 256
    CK_B = (CK_B + CK_A) % 256
```

Fletcher adalah checksum posisional — nilai CK_B bergantung pada urutan byte. Ini mendeteksi burst error lebih baik daripada XOR (yang tidak sensitif terhadap urutan), dan dapat mendeteksi semua error bit tunggal, error bit ganda, dan sebagian besar burst error <16 bit. Komputasi lebih ringan dari CRC — cocok untuk microcontroller 8-bit seperti ATmega328P (Arduino UNO).

### 3.2 UBX-NAV-PVT — Navigation Position Velocity Time

Ini adalah message paling fundamental — menggabungkan data dari beberapa message NMEA ke dalam satu frame compact. Payload minimal 92 byte (versi 1). Setiap field diformat sebagai integer little-endian (LSB first):

**Byte offsets dan interpretasi:**

| Offset | Nama | Tipe | Skala | Penjelasan |
|--------|------|------|-------|------------|
| 0 | iTOW | U4 | ms | GPS time of week (ms). Rollover setiap 604800000ms (1 minggu) |
| 4 | year | U2 | — | UTC year (2026) |
| 6 | month | U1 | — | 1-12 |
| 7 | day | U1 | — | 1-31 |
| 8 | hour | U1 | — | 0-23 |
| 9 | min | U1 | — | 0-59 |
| 10 | sec | U1 | — | 0-60 (leap second) |
| 11 | valid | U1 | flags | Bitmask validity: bit0=validDate, bit1=validTime, bit2=fullyResolved |
| 12 | tAcc | U4 | ns | Time accuracy estimate (nanoseconds) |
| 16 | nano | I4 | ns | Nanoseconds of second |
| 20 | **fixType** | U1 | — | 0=NoFix, 1=DeadReck, 2=2D, **3=3D**, 4=GNSS+DeadReck, 5=TimeOnly |
| 21 | **flags** | U1 | bitmask | bit0=gnssFixOK, bit1=diffSoln, **bits6:7=carrSoln** |
| 22 | flags2 | U1 | — | |
| 23 | numSV | U1 | — | Number of satellites used |
| 24 | **lon** | I4 | 1e-7° | Longitude |
| 28 | **lat** | I4 | 1e-7° | Latitude |
| 32 | height | I4 | mm | Height above ellipsoid |
| 36 | **hMSL** | I4 | mm | Height above Mean Sea Level |
| 40 | **hAcc** | U4 | mm | **Horizontal accuracy estimate** |
| 44 | vAcc | U4 | mm | Vertical accuracy estimate |
| 48 | velN | I4 | mm/s | North velocity |
| 52 | velE | I4 | mm/s | East velocity |
| 56 | velD | I4 | mm/s | Down velocity |
| 60 | **gSpeed** | I4 | mm/s | Ground speed (2D) |
| 64 | **headMot** | I4 | 1e-5° | Heading of motion |
| 68 | headVeh | I4 | 1e-5° | Heading of vehicle |
| 72 | pDOP | U2 | 0.01 | Positional DOP |
| 76+ | ... | | | Additional fields |

**carrSoln flags (byte [21], bits 6:7):**

Ini adalah informasi yang **tidak tersedia di NMEA** dan fundamental untuk monitoring RTK:

| Bits 6:7 | Nama | Arti |
|-----------|------|------|
| 00 | No carrier | Menggunakan pseudorange saja — single/DGPS |
| 01 | Float | Carrier phase tracked, integer ambiguity not resolved |
| 10 | Fixed | **Integer ambiguity resolved — RTK FIX** |
| 11 | Reserved | |

**Konversi satuan di Arduino Base:**

```cpp
// Lat/Lon: int32 1e-7 derajat
rover.lat = parseLong(&payload[28]);      // 1068271532 → 106.8271532°
rover.lon = parseLong(&payload[24]);

// Altitude: int32 mm MSL
rover.alt_mm = parseLong(&payload[36]);   // 15420 → 15.420m

// hAcc: uint32 mm
rover.hAcc_mm = (uint16_t)(parseULong(&payload[40]) & 0xFFFF);

// Speed: int32 mm/s → km/h
rover.speed_kmh = (uint16_t)((gSpeed_mms * 36) / 10000);

// Heading: int32 1e-5 derajat
rover.heading = headMot_1e5 / 100000;     // modulo 360
```

### 3.3 UBX-NAV-RELPOSNED — Relative Positioning

Message ini hanya tersedia dalam mode RTK (Base Station aktif) dan memberikan **vektor baseline** dari Base Station ke Rover dalam sistem koordinat NED (North-East-Down) lokal di posisi Base:

| Offset | Nama | Tipe | Skala | Penjelasan |
|--------|------|------|-------|------------|
| 0 | version | U1 | — | Message version |
| 1 | reserved | U1 | — | |
| 2 | refStationId | U2 | — | ID stasiun referensi (Base) |
| 4 | iTOW | U4 | ms | GPS time of week |
| 8 | **relPosN** | I4 | cm | Baseline North component |
| 12 | **relPosE** | I4 | cm | Baseline East component |
| 16 | **relPosD** | I4 | cm | Baseline Down component |
| 20 | relPosLength | I4 | cm | Baseline length (3D) |
| 24 | accHeading | I4 | 1e-5° | Accuracy of heading |
| 32 | flags | U4 | bitmask | bit0=gnssFixOK, bit1=diffSoln, **bit2=relPosValid**, bits3:4=carrSoln |

**Penggunaan di Coastal Driffter:**

```cpp
rover.relN_cm = parseLong(&payload[8]);   // relPosN: 4215 → 42.15m North dari Base
rover.relE_cm = parseLong(&payload[12]);  // relPosE: 1870 → 18.70m East dari Base
```

Ini memberi tahu kita seberapa jauh Rover dari Base — langsung dalam satuan meter tanpa konversi koordinat. Dashboard menampilkan nilai ini sebagai `relN_m` dan `relE_m` di panel.

### 3.4 Mengapa UBX — Bukan NMEA — untuk Arduino Base

1. **Binary = efisien**: 92 byte NAV-PVT vs ~400 byte NMEA equivalent (RMC + GGA + GLL + VTG + GSA). Untuk Arduino dengan RAM terbatas (2KB ATmega328P), buffer kecil berarti kehilangan byte lebih sedikit.

2. **Presisi numerik penuh**: Semua field adalah integer dengan skala presisi penuh (1e-7° untuk lat/lon, mm untuk altitude/akurasi). Tidak ada truncation akibat konversi float→string→float.

3. **Akses ke carrSoln**: Kita bisa membedakan RTK FLOAT vs FIXED dengan pasti — bukan hanya melihat fix quality NMEA yang kadang kurang akurat.

4. **RELPOSNED**: Informasi baseline vector yang tidak ada di NMEA. Tanpa ini, dashboard tidak bisa menunjukkan jarak Rover dari Base secara langsung — harus menghitung Haversine setiap saat (yang tidak setara dengan baseline GNSS yang dihitung dari carrier phase).

5. **Fletcher checksum**: Deteksi error lebih kuat dari XOR NMEA — penting untuk link wireless via XBee yang mungkin memiliki burst error.

---

## 4. RTK Theory

### 4.1 Dari Single-Point ke Centimeter-Level

Untuk memahami mengapa RTK begitu revolusioner, kita perlu memahami bahwa semua error dalam pengukuran GPS (ionosfer, troposfer, clock, orbit) bersifat **berkorelasi secara spasial** — artinya, dua receiver yang berdekatan (<10 km) akan mengalami error yang hampir identik untuk satelit yang sama.

**Prinsip Differential GPS (DGPS)**:

Base Station (posisi diketahui dengan presisi) menghitung error pseudorange untuk setiap satelit:

```
PRC = ρ_observed − ρ_true
```

di mana ρ_true dihitung dari posisi Base yang diketahui + posisi satelit dari ephemeris. PRC (PseudoRange Correction) dikirim ke Rover melalui link radio (RTCM / SBAS / beacon). Rover mengoreksi pseudorange-nya:

```
ρ_corrected = ρ_rover − PRC
```

DGPS mengurangi error dari 5-15m menjadi 0.5-3m — sangat baik, tapi tidak cukup untuk surveying presisi.

### 4.2 Carrier Phase — Lompatan Kuantum dari Meter ke Centimeter

GPS carrier wave L1 berfrekuensi 1575.42 MHz dengan panjang gelombang λ = c/f ≈ 0.1903 meter (19.03 cm untuk L1, 24.42 cm untuk L2). Receiver modern dapat mengukur fase carrier wave dengan presisi **±1-2 mm** (1% dari panjang gelombang).

Pengukuran carrier phase dinyatakan sebagai:

```
Φ = ρ/λ + N
```

di mana:
- `Φ` = fase yang diukur (dalam siklus) — precision ~0.01 siklus
- `ρ` = jarak geometrik satelit-ke-receiver
- `λ` = panjang gelombang carrier
- `N` = **integer ambiguity** — jumlah siklus penuh yang tidak diketahui

Ketika receiver pertama kali mengunci sinyal (lock-on), ia tahu fase fraksional (Φ mod 1) dengan sangat presisi, tetapi tidak tahu berapa banyak siklus penuh (N) antara satelit dan antena. N adalah integer — harus dicari.

### 4.3 Integer Ambiguity Resolution — The LAMBDA Method

Proses resolusi integer ambiguity adalah tantangan komputasional utama dalam RTK. Secara matematis, kita memiliki sistem persamaan yang overdetermined (banyak satelit):

```
y = A·a + B·b + ε
```

di mana:
- `y` = vektor observasi (double-differenced carrier phase)
- `a` = vektor integer ambiguities (unknown integers)
- `b` = vektor baseline (unknown real-valued position)
- `A, B` = matriks desain
- `ε` = noise

**Double-differencing**: Untuk menghilangkan receiver clock error dan satellite clock error, kita membentuk perbedaan ganda:

1. Single-difference antar-receiver: (Φ_rover − Φ_base) untuk satelit yang sama → mengeliminasi satellite clock error
2. Double-difference antar-satelit: single-diff(sat1) − single-diff(sat_ref) → mengeliminasi receiver clock error

Hasilnya adalah observasi yang hanya mengandung: geometri baseline, integer ambiguities, dan residual atmospheric + noise. Tahap resolusi integer:

1. **Float solution**: Estimasi real-valued dari ambiguitas (seolah-olah N adalah float, bukan integer). Solusi ini adalah RTK FLOAT — akurasi 10-50cm.

2. **Integer search**: Mencari integer N yang meminimalkan residu. Ruang pencarian adalah integer grid berdimensi (n_satellites − 1) — secara komputasi mahal. Metode LAMBDA (**L**east-squares **AMB**iguity **D**ecorrelation **A**djustment) melakukan transformasi Z untuk mende-korelasi ambiguitas, memperkecil search space secara dramatis.

3. **Validation**: Membandingkan solusi integer terbaik vs runner-up menggunakan RATIO test: `ratio = RSS(second_best) / RSS(best)`. Ratio > 3.0 umumnya dianggap confident fix.

Di ZED-F9P, proses ini terjadi secara otomatis di dalam receiver firmware — Arduino tidak perlu melakukan perhitungan LAMBDA. Yang kita lakukan hanyalah membaca carrSoln flag di UBX-NAV-PVT.

### 4.4 RTK FLOAT vs RTK FIX — Maknanya di Lapangan

**RTK FLOAT** (carrSoln=1, fix=5 di NMEA GGA):
- Ambiguitas diestimasi sebagai real-valued — belum integer
- Akurasi: 10-50 cm horizontal
- Terjadi saat: Base baru menyala (belum cukup data untuk resolve), multipath tinggi, cycle slip baru terjadi, atau baseline terlalu panjang
- Ciri di dashboard: badge kuning "RTK FLOAT", hAcc ~100-500mm

**RTK FIXED** (carrSoln=2, fix=4 di NMEA GGA):
- Ambiguitas resolved sebagai integer — akurasi maksimal
- Akurasi: 1-2 cm + 1 ppm × baseline_km
- Baseline 1 km → 1-2 cm + 1 mm = ~1-3 cm
- Baseline 10 km → 1-2 cm + 1 cm = ~2-3 cm
- Ciri di dashboard: badge hijau "RTK FIX (1-2 cm)", hAcc ~8-20mm

**Transisi FLOAT → FIXED** terjadi dalam 5-30 detik setelah Base mengirim RTCM3 — lebih cepat jika geometri satelit baik, sinyal kuat, dan baseline pendek.

**Degradasi FIXED → FLOAT** terjadi akibat:
- **Cycle slip**: receiver kehilangan lock carrier phase sesaat (sinyal terblokir oleh bangunan, kepala, atau multipath ekstrem) → integer counter melompat → perlu re-initialization
- **Loss of RTCM3**: Base mati atau link XBee terputus → Rover kehilangan koreksi → kembali ke single (fix=1)
- **Jumlah satelit rendah**: <5 satelit common view — tidak cukup untuk resolve ambiguitas

Di Coastal Driffter (lingkungan laut), cycle slip sering terjadi karena: pantulan air menghasilkan multipath kuat, pergerakan buoy mengubah geometri terus-menerus, dan gelombang dapat menutupi antena sesaat.

### 4.5 Baseline Distance — Pengaruh Jarak Base-Rover

RTK single-baseline bekerja berdasarkan asumsi bahwa error atmosfer di Base dan Rover berkorelasi sempurna (identik). Realitanya, korelasi menurun seiring jarak. Efek pada RTK:

| Jarak Baseline | Perbedaan Atmosfer | Dampak | Maks Baseline |
|---|---|---|---|
| <1 km | <1 mm iono, <1 mm tropo | Dapat diabaikan — fix mudah | — |
| 1-5 km | ~2-5 mm iono, ~1-3 mm tropo | Fix masih mudah | Coastal Driffter ideal |
| 5-10 km | ~5-10 mm iono, ~3-10 mm tropo | Fix mungkin, waktu inisialisasi lebih lama | — |
| 10-20 km | ~10-25 mm iono, ~10-30 mm tropo | Fix sulit — perlu model atmosfer | Single-baseline limit |
| >20 km | Tidak berkorelasi signifikan | Fix tidak mungkin tanpa network RTK atau PPP-RTK | — |

ZED-F9P rekomendasi baseline <10 km untuk RTK operasional. Coastal Driffter dengan baseline 100m-5km berada di sweet spot.

---

## 5. RTCM 3.x Correction Data

### 5.1 Standar RTCM SC-104

RTCM (Radio Technical Commission for Maritime Services), Special Committee 104, adalah badan standar yang mendefinisikan format untuk transmisi data koreksi GNSS diferensial. Standar RTCM digunakan sebagai jembatan komunikasi antara Base dan Rover — berbeda dengan format UBX atau NMEA yang merupakan format output receiver.

Sejarah singkat:
- RTCM v2.x (~1990): format ASCII-ish untuk DGPS pseudorange correction. Hanya GPS, hanya C/A code.
- RTCM v3.0 (2004): format binary compact — efisiensi bandwidth tinggi. Mulai mendukung carrier phase correction untuk RTK.
- RTCM v3.1 (2007): menambahkan Network RTK messages (VRS, FKP, MAC)
- RTCM v3.2 (2010): Multiple Signal Messages (MSM) — mendukung multi-konstelasi, multi-frekuensi
- RTCM v3.3 (2013+): BeiDou B1/B2, Galileo E1/E5, dan SBAS support

Sistem Coastal Driffter menggunakan RTCM v3.x (MSM4-7) yang diproduksi oleh Base ZED-F9P dalam mode Survey-In dan dikirim melalui XBee ke Rover.

### 5.2 Multiple Signal Messages (MSM)

MSM adalah format encoding compact untuk data pengamatan GNSS mentah — menggantikan message RTCM v2 yang verbose. Ada 7 tipe MSM per konstelasi:

| Type | GPS | GLONASS | Galileo | BeiDou | Isi |
|---|---|---|---|---|---|
| MSM4 | 1074 | 1084 | 1094 | 1124 | Compact pseudorange + phaseRange |
| MSM5 | 1075 | 1085 | 1095 | 1125 | Compact pseudorange + phaseRange + CNR + Doppler |
| MSM6 | 1076 | 1086 | 1096 | 1126 | Full pseudorange + phaseRange + CNR |
| MSM7 | 1077 | 1087 | 1097 | 1127 | Full pseudorange + phaseRange + CNR + Doppler |

MSM4 adalah yang paling compact — cocok untuk link bandwidth rendah (XBee 9600 bps). Mengandung: rough pseudorange (dengan modulo ambiguity), fine pseudorange, carrier phase, dan lock time indicator untuk setiap sinyal dari setiap satelit. Jumlah byte per satelit sangat efisien karena menggunakan encoding diferensial: sebagian besar field disimpan sebagai delta dari reference value.

Contoh bandwidth MSM untuk 12 satelit GPS + GLONASS: MSM4 ≈ 0.8-1.2 KB/s @ 1Hz. XBee 9600 bps = 1200 bytes/s — ini cukup untuk 1Hz koreksi + header margin.

### 5.3 RTCM 1005/1006 — Station Coordinates

Sebelum Rover bisa menghitung posisinya relatif terhadap Base, ia perlu tahu **di mana Base berada**. Message RTCM 1005 atau 1006 membawa koordinat Antenna Reference Point (ARP) Base Station dalam ECEF (Earth-Centered Earth-Fixed) XYZ:

- Message 1005: koordinat ARP + antenna height (4 byte ECEF X, Y, Z komponen)
- Message 1006: lebih lengkap — menambahkan antenna descriptor, GPS indicator, GLONASS indicator, dll

Koordinat ECEF Base inilah yang digunakan Rover untuk menghitung baseline vector (RELPOSNED). Tanpa message ini, Rover tidak tahu posisi Base — dan RELPOSNED akan tetap nol.

### 5.4 Alur Data RTCM3 di Coastal Driffter

```
1. Base GNSS → TMODE3 Survey-In selesai → auto-start RTCM3 generation
2. RTCM3 binary stream → Base UART1 (38400 bps)
3. Arduino Base → gnssSerial.read() → radioSerial.write() [relay byte-by-byte]
4. XBee Base TX → wireless 2.4GHz → XBee Rover RX
5. XBee Rover → Rover GNSS UART1 RX
6. Rover GNSS → mendeteksi RTCM3 → mode RTK → NAV-PVT dengan carrSoln = 2 (FIXED)
7. Rover GNSS → UBX-NAV-PVT + NAV-RELPOSNED → XBee Rover TX → Base → Dashboard
```

Langkah kunci di atas: Arduino Base tidak mem-parsing RTCM3 — hanya melakukan forwarding byte-by-byte (`radioSerial.write(b)`). Ini adalah keputusan desain yang baik karena: (a) tidak perlu implementasi parser RTCM3 yang kompleks di Arduino, (b) tidak ada risiko corrupt data akibat parsing error, dan (c) latency forwarding minimal (<1ms) — penting untuk data real-time.

### 5.5 Kenapa RTCM3 Tidak Perlu 115200 Baud

Poin kebingungan umum: kenapa XBee di 9600 bps cukup untuk RTK? RTCM3 MSM4 ~800 byte/saat @ 1Hz. Ini hanya ~15% dari kapasitas 9600 bps (1200 byte/s). Telemetri uplink dari Rover (UBX ~200 byte/s) menggunakan sisa bandwidth. Tidak ada bottleneck.

Jika di masa depan diperlukan rate lebih tinggi (5Hz RTK) atau multi-constellation penuh (MSM7), bandwidth XBee mungkin menjadi bottleneck — dalam kasus tersebut, XBee bisa ditingkatkan ke 19200 atau 38400 baud.

---

## 6. Fix Status & Quality Metrics

### 6.1 Interpretasi Fix Quality Lengkap

Fix quality adalah indikator **seberapa terpercaya** solusi posisi saat ini. Bukan hanya tentang akurasi absolut, tetapi juga tentang mekanisme yang digunakan receiver:

**Fix 0 — No Fix**: Receiver tidak memiliki solusi posisi sama sekali. Terjadi saat:
- Cold start (baru dinyalakan): almanac dan ephemeris belum di-download dari satelit — butuh 30-60 detik untuk first fix
- Antena tidak terpasang atau indoor (atenuasi >30dB melalui atap beton)
- Jumlah satelit <4 (tidak cukup untuk trilaterasi 3D)
- Di dashboard: badge merah "NO FIX", lat/lon 0 atau tetap konstan di posisi terakhir

**Fix 1 — Autonomous / SINGLE / 3D Fix**: Solusi standalone tanpa koreksi eksternal. 4+ satelit dilacak, navigasi normal. Akurasi 3-15m horizontal (95%). Ini adalah mode normal ketika Base tidak tersedia — Rover sendirian dengan mode NMEA di dashboard (baud 38400).

**Fix 2 — DGPS**: Koreksi dari sumber diferensial non-RTK. Bisa berasal dari:
- SBAS (Satellite-Based Augmentation System): satelit geostationer yang menyiarkan koreksi — WAAS (Amerika Utara), EGNOS (Eropa), MSAS (Jepang), GAGAN (India)
- DGPS Beacon: stasiun terrestrial di frekuensi 283-325 kHz, biasanya untuk navigasi laut coastal
- Di Indonesia: coverage SBAS terbatas — fix 2 jarang terjadi. EGNOS tidak menjangkau Asia Tenggara. MSAS Jepang coverage bisa mencapai Indonesia timur.

**Fix 4 — RTK FIXED**: Integer ambiguity resolved — akurasi tertinggi. Base Station aktif, RTCM3 diterima Rover. Akurasi 1-2cm + 1ppm. Kondisi ideal: >6 satelit common view, HDOP <2, baseline <10km, sinyal stabil tanpa cycle slip. Badge hijau di dashboard.

**Fix 5 — RTK FLOAT**: Base aktif tapi integer ambiguity belum resolved. Akurasi 10-50cm. Terjadi jika: Base baru saja mulai (belum cukup data), geometri buruk, atau cycle slip baru terjadi. Biasanya transisi ke FIXED dalam 10-60 detik. Badge kuning di dashboard.

### 6.2 hAcc — Horizontal Accuracy Estimate

UBX-NAV-PVT field hAcc (byte offset 40, 4 byte uint, satuan mm) adalah estimasi CEP (Circular Error Probable) — radius lingkaran yang mengandung 50% dari kemungkinan posisi. Bukan 95% (2-drms) — untuk konversi ke 95%: kalikan ~2.4.

hAcc dihitung receiver dari:
- Covariance matrix posisi (dari least-squares solution)
- Signal quality (C/N0 dari setiap satelit)
- Jumlah satelit yang digunakan
- DOP
- Jenis fix (carrier vs pseudorange)

Nilai khas:

| Fix Type | hAcc (mm) | Akurasi 95% |
|----------|-----------|-------------|
| SINGLE | 1000-5000 | 2.5-12m |
| DGPS | 200-1000 | 0.5-2.5m |
| RTK FLOAT | 30-200 | 7-50cm |
| RTK FIXED | 5-20 | 1.2-5cm |

Dashboard Coastal Driffter menampilkan hAcc dalam sentimeter: `hAcc_cm = hAcc_mm / 10`.

### 6.3 Cycle Slip — Musuh RTK di Lapangan

Cycle slip adalah diskontinuitas dalam pengukuran carrier phase — receiver "kehilangan hitungan" siklus karena sinyal terblokir sesaat. Ini terjadi jika sinyal SNR turun di bawah threshold tracking (biasanya ~25 dB-Hz) untuk beberapa milidetik. Penyebab umum:

- **Multipath ekstrem**: Interferensi destruktif dari pantulan air/bangunan menyebabkan deep fade pada sinyal direct path
- **Objek lewat di atas antena**: Burung, drone, atau (di Coastal Driffter) gelombang buih di atas antena
- **Dinamika tinggi**: Akselerasi atau jerk tinggi pada buoy saat ombak besar → PLL (Phase Lock Loop) kehilangan lock
- **Low elevation satellite**: Sinyal lemah (<30 dB-Hz) lebih rentan cycle slip

Setelah cycle slip, integer counter melompat (ΔN bukan nol) — seluruh perhitungan ambiguitas menjadi invalid. Receiver harus melakukan re-initialization: turun dari RTK FIX ke FLOAT atau SINGLE, kemudian re-converge ke FIX (5-30 detik). Di lingkungan pantai dengan multipath tinggi, cycle slip bisa terjadi setiap beberapa menit — ini adalah tantangan utama RTK di lingkungan marine.

Indikator cycle slip di dashboard: fix status tiba-tiba turun dari "RTK FIX" ke "RTK FLOAT" lalu kembali lagi dalam beberapa detik — ini normal dan ekspektasi di coastal environment.

### 6.4 Convergence Time

Waktu yang dibutuhkan receiver untuk transisi dari SINGLE ke RTK FIX setelah pertama kali menerima RTCM3. Faktor yang mempengaruhi:

| Faktor | Dampak |
|---|---|
| Jumlah satelit | 5-6 satelit → lambat (30-60s), 10+ satelit → cepat (5-10s) |
| Multi-constellation | GPS only → lebih lambat; GPS+GLO+GAL+BDS → jauh lebih cepat karena lebih banyak observasi |
| Baseline length | <1km → sangat cepat; >5km → lebih lambat |
| Signal strength | C/N0 >40 dB-Hz → cepat; <35 dB-Hz → lambat |
| Multipath | Rendah → cepat; tinggi → bisa gagal converge |

Convergence typical ZED-F9P dalam kondisi coastal terbuka: 10-30 detik untuk first fix.

---

## 7. Base Station & Survey-In

### 7.1 Peran Base Station

Base Station adalah referensi statis yang memungkinkan seluruh sistem RTK. Ia menyediakan dua hal esensial ke Rover:

1. **Observasi GNSS mentah** (pseudorange + carrier phase + CNR + Doppler untuk setiap satelit) — dikirim sebagai RTCM3 MSM messages
2. **Posisi Base yang akurat** — dikirim sebagai RTCM3 1005/1006

Rover menggunakan data ini dengan cara: mengurangi observasinya sendiri dengan observasi Base → menghilangkan common errors (satellite clock, ionosphere, troposphere) → mendapatkan baseline vector → menambahkannya ke posisi Base → mendapatkan posisi absolut Rover yang akurat.

**Kunci**: Keakuratan posisi absolut Rover **bergantung pada keakuratan posisi absolut Base**. Jika Base meleset 2 meter, Rover juga akan meleset 2 meter — meskipun baseline (jarak relatif Rover terhadap Base) bisa tetap akurat hingga cm-level. Inilah mengapa Survey-In penting — untuk mendapatkan posisi Base seakurat mungkin sebelum broadcasting koreksi.

### 7.2 TMODE3 Survey-In — Proses

ZED-F9P memiliki mode Time Mode 3 (TMODE3) — mode Survey-In — di mana receiver bertindak sebagai Base Station dan **menentukan posisinya sendiri secara otonom** melalui pengukuran posisi berulang yang dirata-ratakan sepanjang waktu.

**Proses matematis**: Base receiver melakukan positioning autonomous (single-point) setiap epoch (1 detik). Posisi setiap epoch, `x_i`, ditulis ke buffer sirkular. Moving average dihitung:

```
x̄_N = (1/N) Σᵢ₌₁ᴺ x_i
```

Seiring bertambahnya N, rata-rata konvergen ke nilai "true" dengan laju σ/√N — di mana σ adalah error standar single-epoch (~5-10m untuk autonomous). Untuk mencapai akurasi rata-rata <3m: N ≈ (σ/3)² = (5/3)² ≈ 3 epoch (3 detik). Tapi ini minimum ideal — dalam praktik, noise berkorelasi temporal (slow-varying multipath, atmospheric drift) memperlambat konvergensi, sehingga diperlukan 60-300 detik.

Selain waktu minimum, ada threshold akurasi: `Required Position Accuracy` — survey berhenti jika: (waktu ≥ minimum) DAN (σ_x̄ ≤ accuracy_threshold). Jika setelah waktu minimum akurasi belum tercapai, survey melanjutkan sampai threshold terpenuhi atau dihentikan manual.

### 7.3 Konfigurasi Survey-In

| Parameter | Rekomendasi | Keterangan |
|---|---|---|
| Min Observation Time | 60 detik | Minimal untuk operasional — korbankan akurasi ~3m |
| Min Observation Time (surveying) | 300-600 detik | Untuk surveying presisi — akurasi ~1m |
| Required Accuracy | 3.0m | Cukup untuk RTK operasional (baseline pendek) |
| Required Accuracy (presisi) | 1.0m | Lebih baik untuk baseline panjang |

Tradeoff: waktu vs akurasi. Di lapangan Coastal Driffter:
- Setting 60s + 3.0m = Base siap dalam 1 menit, akurasi cukup untuk tracking buoy
- Setting 300s + 1.0m = Base siap dalam 5 menit, akurasi lebih baik untuk surveying post-deployment

**Yang terjadi jika Base bergerak selama Survey-In**: posisi rata-rata akan mengikuti pergerakan → koordinat base station tidak lagi valid → seluruh posisi Rover akan offset sebesar pergerakan Base. Oleh karena itu: **Base harus tetap stasioner** — diletakkan di tripod, rooftop, atau dermaga tetap. Jangan dipegang tangan.

### 7.4 Alur Operasional Base di Coastal Driffter

1. Nyalakan Base GNSS + Arduino Base (USB) + XBee
2. Base GNSS boot → mode Survey-In → LED berkedip menandakan surveying
3. Arduino Base mulai relay data dari Base GNSS ke XBee, dan mendengarkan Rover
4. Setelah ~60-120 detik: Survey-In selesai → Base GNSS mulai output RTCM3 + UBX-NAV-PVT
5. Arduino Base membaca UBX-NAV-PVT untuk posisi Base, dan relay RTCM3 byte-by-byte
6. Dashboard menerima JSON dengan base.lat/base.lon dari UBX (bukan hardcode Monas)

**Sebelum Survey-In selesai**: Base GNSS belum mengirim posisi valid → Arduino tetap mengirim JSON, tapi base.lat/base.lon menggunakan nilai default (Monas, -6.1753924, 106.8271532). Begitu Survey-In selesai dan UBX-NAV-PVT mulai mengalir, base.lat/base.lon ter-update ke posisi aktual.

---

## 8. RELPOSNED — Relative Positioning

### 8.1 Sistem Koordinat NED

NED (North-East-Down) adalah sistem koordinat lokal Cartesian dengan origin di posisi referensi (dalam RTK, origin = posisi Base Station). Sistem ini mendefinisikan tiga sumbu ortogonal:

- **North (N)**: Arah utara geodetik — sepanjang meridian lokal
- **East (E)**: Arah timur geodetik — sejajar paralel lokal
- **Down (D)**: Ke bawah sepanjang normal ellipsoid — kira-kira searah gravitasi

NED adalah **local tangent plane** — aproksimasi permukaan bumi datar di sekitar Base. Ini valid untuk baseline pendek (<10 km). Untuk baseline panjang, perlu memperhitungkan kelengkungan bumi — dilakukan secara internal oleh algoritma RTK.

Satu hal penting: **NED bukan kompas magnetik**. North adalah true north (sepanjang sumbu rotasi bumi), bukan magnetic north. Ini dihitung dari geometri ellipsoid WGS84, bukan dari magnetometer.

### 8.2 Transformasi ECEF → NED

Posisi dihitung dalam ECEF Cartesian (X, Y, Z) oleh receiver, kemudian ditransformasikan ke NED untuk pelaporan:

```
Untuk origin di (φ₀, λ₀) — posisi Base Station:
Matriks rotasi: R(φ₀, λ₀) = [
    [-sin(φ₀)cos(λ₀),  -sin(φ₀)sin(λ₀),   cos(φ₀)],
    [-sin(λ₀),          cos(λ₀),            0       ],
    [-cos(φ₀)cos(λ₀),  -cos(φ₀)sin(λ₀),   -sin(φ₀)]
]

Baseline NED = R(φ₀, λ₀) × (X_rover − X_base)
```

### 8.3 Implementasi di Coastal Driffter

UBX-NAV-RELPOSNED dari Rover memberikan:

```
relPosN:  4215  → 42.15m (Rover berada 42.15m di utara Base)
relPosE:  1870  → 18.70m (Rover berada 18.70m di timur Base)
relPosD:  -250  → -2.50m (Rover 2.50m di atas Base — negatif berarti lebih tinggi)
```

Dari sini bisa dihitung:

- **Baseline distance horizontal**: √(relPosN² + relPosE²) = √(42.15² + 18.70²) = √(2311.62) = **48.08m**
- **Azimuth dari Base ke Rover**: atan2(relPosE, relPosN) = atan2(18.70, 42.15) = **23.9°** (timur laut)

Dashboard menampilkan:
- `relN_m` dan `relE_m` di panel dinamika
- Baseline distance dihitung via Haversine dari koordinat absolut (bukan dari RELPOSNED — keduanya seharusnya konsisten)

### 8.4 Kapan RELPOSNED Valid?

| Kondisi | relPosValid | Nilai relPosN, relPosE |
|---|---|---|
| RTK FIX | ✅ true (flag bit 2 = 1) | Nilai valid, akurasi cm-level |
| RTK FLOAT | ✅ true (masih ada baseline) | Nilai valid, akurasi 10-50cm |
| SINGLE | ❌ false | 0 (tidak ada baseline tanpa referensi) |
| Base mati | ❌ false | 0 |

---

## 9. Dilution of Precision (DOP)

### 9.1 Konsep — Mengapa Geometri Satelit Krusial

IDOP mengukur seberapa besar ketidakpastian pengukuran diperkuat menjadi ketidakpastian posisi akibat geometri satelit yang tidak ideal. Analogi intuitif: jika tiga satelit berkelompok di area langit yang kecil, perubahan kecil dalam pseudorange dapat menghasilkan perubahan besar dalam posisi — area perpotongan sphere/sphere sangat luas (DOP tinggi). Sebaliknya, satelit yang tersebar merata di seluruh langit menghasilkan perpotongan yang tajam — DOP rendah.

Secara matematis, DOP diturunkan dari matriks kovarians posisi:

```
C_x = (Hᵀ H)⁻¹ σ²
```

di mana H adalah matriks desain (N×4) dengan setiap baris adalah directional cosine dari receiver ke satelit:

```
H_i = [dx_i, dy_i, dz_i, 1]
```

DOP values adalah akar dari elemen diagonal C_x (dalam satuan meter per meter measurement error):

```
GDOP = √(σ²_x + σ²_y + σ²_z + σ²_ct) / σ_meas
PDOP = √(σ²_x + σ²_y + σ²_z) / σ_meas
HDOP = √(σ²_E + σ²_N) / σ_meas
VDOP = σ_U / σ_meas
TDOP = σ_ct / σ_meas
```

### 9.2 Interpretasi DOP di Lapangan

| DOP | Rating | Karakteristik Geometri | Dampak |
|-----|--------|------------------------|--------|
| 0.8-1.0 | Ideal | 4+ satelit di kuadran berbeda, elevation mix (satu dekat zenith, tiga di ~30° horizon) | Hampir tidak ada amplifikasi error |
| 1-2 | Excellent | Geometri sangat baik — satelit tersebar merata, >8 satelit tersedia | RTK FIX mudah, error amplified <2x |
| 2-5 | Good | Geometri baik — tipikal langit terbuka | Cukup untuk RTK, mungkin perlu waktu lebih lama untuk FIX |
| 5-10 | Moderate | Satelit mulai berkelompok, atau banyak di elevasi rendah (<10°) | RTK masih mungkin tapi sulit, FLOAT mungkin bertahan lama |
| 10-20 | Poor | Hanya ~4-5 satelit, geometri sangat asymmetris (bisa semua di satu sisi bangunan) | RTK tidak mungkin, akurasi single degraded ke >10m |
| >20 | Unusable | <4 satelit atau semua di area langit <20° | Tidak bisa digunakan untuk apapun |

**Hubungan DOP dengan akurasi absolut**:

```
σ_pos = DOP × σ_meas
```

di mana σ_meas adalah akurasi pengukuran:
- Pseudorange (C/A code): σ_meas ≈ 1-3m → σ_pos (SINGLE, HDOP=2) ≈ 2-6m
- Carrier phase (RTK): σ_meas ≈ 2-5mm → σ_pos (RTK FIX, HDOP=2) ≈ 4-10mm

### 9.3 DOP di Lingkungan Coastal

Lingkungan laut lepas (tanpa obstruksi) sebenarnya ideal untuk DOP — langit terbuka 360°, tidak ada bangunan atau pohon. DOP di lautan terbuka biasanya 1-2 (excellent) karena banyak satelit terlihat. Tapi ada trade-off:

- **Elevasi rendah**: satelit di bawah 10° elevation mengalami multipath parah dari permukaan laut → receiver akan menolak satelit ini (elevation mask) → mengurangi jumlah satelit → meningkatkan DOP
- **Elevation mask optimal**: 10-15° untuk coastal environment — mengorbankan beberapa satelit (naikin DOP 0.2-0.5) tapi mengurangi multipath secara drastis → lebih sedikit cycle slip
- **Laut bergelombang**: antena di buoy bergerak naik-turun → setiap satelit mengalami perubahan elevasi efektif → DOP berfluktuasi

---

## 10. Serial Communication

### 10.1 UART Fundamentals

UART (Universal Asynchronous Receiver-Transmitter) adalah protokol komunikasi serial asynchronous — tidak ada clock line terpisah; kedua pihak harus menyetujui baud rate di muka. Setiap frame terdiri dari:

```
[START] [D0] [D1] [D2] ... [D7] [PARITY?] [STOP]
```

- START bit = 0 (line idle HIGH, transisi LOW menandakan start)
- Data bits: 5-8 bit, LSB first (hampir selalu)
- Parity (optional): none / even / odd — semua GNSS menggunakan none
- STOP bit(s): 1 atau 2 — line kembali HIGH

Format paling umum (dan yang digunakan di seluruh sistem Coastal Driffter): **8N1** — 8 data bits, no parity, 1 stop bit. Total 10 bit per byte yang ditransmisikan.

**Baud vs bit rate**: Baud = symbol rate (perubahan sinyal per detik). Untuk UART binary (2-level), baud = bit rate. Jadi di 9600 baud, bit rate = 9600 bps, byte rate (8N1) = 9600/10 = 960 byte/s.

### 10.2 Baud Rate dalam Sistem Coastal Driffter

| Koneksi | Baud | Byte Rate | Data Rate | Alasan |
|--------|------|-----------|-----------|--------|
| GNSS → Arduino | 38400 | 3840 B/s | UBX ~100B @ 1Hz = 800bps + NMEA ~400B @ 1Hz = 3200bps total ~4000bps | 38400 = safety margin ≥10x |
| Arduino ↔ XBee | 9600 | 960 B/s | RTCM3 ~0.8KB/s + telemetry uplink ~0.2KB/s = ~1KB/s total | 9600 margin tipis (~15%) tapi cukup untuk 1Hz RTK + 5Hz telemetry |
| Arduino → PC | 115200 | 11520 B/s | JSON ~250B @ 5Hz = 1250B/s | Headroom besar — tidak akan pernah bottleneck |

### 10.3 Hardware Serial vs SoftwareSerial

**Hardware Serial (ATmega328P USART)**:

- USART peripheral dedicated — shift register hardware untuk TX/RX
- Double-buffered: UDR register + shift register → saat CPU sibuk, byte berikutnya tetap bisa diterima
- RX interrupt-driven — byte yang diterima disimpan ke buffer hardware (2-level FIFO) lalu trigger ISR
- **Kinerja**: Dapat menerima data kontinu pada baud rate hingga 1 Mbps tanpa kehilangan byte. Tidak membebani CPU.

Digunakan di Arduino UNO untuk pin 0 (RX) dan pin 1 (TX) — inilah jalur komunikasi utama.
Di Base: Hardware Serial = USB ke PC (melalui chip CH340/FTDI).

**SoftwareSerial (bit-banging)**:

- Diimplementasi seluruhnya dalam software — CPU membaca/menulis pin GPIO secara manual dengan timing presisi
- Untuk RX: pin change interrupt mendeteksi falling edge (start bit) → CPU melakukan delay loop presisi untuk sampling setiap bit di tengah interval
- Selama transmisi 1 byte: interrupt global dimatikan (cli) agar timing tidak terganggu → **semua interrupt lain (termasuk millis(), timer) tertunda hingga 10 bit time**
- Baud rate maksimum praktis: **19200 bps** untuk operasi reliable
- **38400 di SoftwareSerial (seperti yang kita lakukan untuk GNSS Base) adalah borderline**: mungkin kehilangan 1-2 byte per packet sesekali. Acceptable karena: (1) UBX checksum mendeteksi corrupt packet (2) packet berikutnya datang 200ms kemudian (3) posisi bergerak lambat → satu lost packet tidak signifikan

Library alternatif jika diperlukan:
- **AltSoftSerial** (pin 8,9 khusus): menggunakan Timer1 hardware untuk timing → lebih akurat, tidak mematikan interrupt → **38400 reliable**
- **NeoSWSerial**: interrupt-optimized, tidak mematikan interrupt, RX buffer ring dalam software → 38400 bisa reliable

### 10.4 Timing dan Latency

- Latency UART TX 1 byte: 10/baud_rate detik (misal 38400 → 260μs/byte)
- Latency UART RX full NAV-PVT packet (100 byte): ~26ms @ 38400
- Latency XBee serial-to-wireless: ~5-10ms (buffering + RF transmission)
- Latency Arduino processing loop: <1ms per iterasi
- Total end-to-end latency (Base GNSS → Dashboard display): ~40-100ms

Untuk tracking buoy yang bergerak pada 1-2 m/s, latency 100ms = error posisi 10-20cm — dapat diterima.

---

## 11. XBee & ISM Radio

### 11.1 Teknologi IEEE 802.15.4

XBee (Digi International) adalah modul radio berbasis standar IEEE 802.15.4 — dirancang untuk WPAN (Wireless Personal Area Network) dengan fokus pada daya rendah dan jaringan sensor. Karakteristik:

- PHY layer: 2.4 GHz (ISM band, worldwide) atau 868/915 MHz (regional)
- Data rate: 250 kbps (2.4 GHz) atau 40/250 kbps (sub-GHz)
- Modulation: O-QPSK (Offset Quadrature Phase Shift Keying) dengan DSSS (Direct Sequence Spread Spectrum) — setiap 4-bit simbol dikodekan menjadi 32-chip PN sequence → processing gain ~9 dB → lebih immune terhadap interferensi narrowband
- Output power: 1mW (0 dBm) hingga 63mW (+18 dBm) tergantung model
- Sensitivity: −100 dBm @ 9600 bps (XBee S2C), −101 dBm @ 9600 bps (XBee PRO 900HP)
- Range (line-of-sight outdoor): 120m (S2C, 2.4GHz, 1mW), 1.6km (S2C PRO, 63mW), 10km+ (900HP, 250mW)

### 11.2 Link Budget — Mengapa XBee Cukup untuk Coastal Driffter

**Free-Space Path Loss (FSPL)**:

```
FSPL(dB) = 20·log₁₀(d) + 20·log₁₀(f) + 32.45
```

- d dalam meter, f dalam MHz, hasil dalam dB

Contoh untuk 2.4 GHz @ 500m (use case typical):

```
FSPL = 20·log₁₀(500) + 20·log₁₀(2400) + 32.45
     = 20 × 2.699 + 20 × 3.380 + 32.45
     = 53.98 + 67.60 + 32.45
     = 94.0 dB
```

**Link Budget Calculation**:

```
Rx Power = Tx Power + Tx Antenna Gain − FSPL + Rx Antenna Gain − Cable Losses − Fade Margin
```

XBee S2C (onboard chip antenna ~1.5dBi):

```
Rx Power @ 500m = 8dBm + 1.5dBi − 94dB + 1.5dBi − 0 − 10dB(fade margin)
                = 8 + 1.5 − 94 + 1.5 − 10
                = −93 dBm
```

Receiver sensitivity = −100 dBm → **margin 7 dB — adequate**. Dengan margin 7 dB, ada risiko fading di lingkungan multipath tinggi (over water).

Untuk XBee PRO 900MHz @ 1km (power 18dBm, gain 2dBi):

```
FSPL = 20·log₁₀(1000) + 20·log₁₀(900) + 32.45 = 60 + 59.08 + 32.45 = 91.5 dB
Rx Power = 18 + 2 − 91.5 + 2 − 10 = −79.5 dBm
Sensitivity @ 9600 = −101 dBm → margin 21.5 dB — excellent
```

### 11.3 Over-Water Propagation

Lingkungan di atas laut/lautan memiliki karakteristik propagasi RF yang unik:

**Fresnel Zone over water**: Permukaan air adalah reflektor RF yang sangat baik — koefisien refleksi near −1 (phase reversal). Sinyal direct path dan reflected path berinterferensi di receiver. Tergantung perbedaan path length (fungsi ketinggian antena dan jarak), kedua sinyal bisa konstruktif (gain) atau destruktif (deep fade, >20 dB loss).

Zone Fresnel pertama: elipsoid antara TX dan RX di mana perbedaan path length = λ/2. Untuk 2.4 GHz (λ=0.125m) di 500m: radius Fresnel zone tengah ≈ √(λ×d/2) / 2 ≈ 5.6m. Jika antena lebih rendah dari 5.6m di atas air, refleksi masuk ke zone pertama → fading parah!

Ini kenapa **antena XBee sebaiknya diletakkan setinggi mungkin** dari permukaan air — minimal 1-2 meter — untuk mengurangi efek pantulan laut. Di buoy kecil, ini sulit → fading periodik mungkin terjadi seiring buoy bergerak relatif terhadap gelombang.

### 11.4 Mode Transparent (AT Mode)

Coastal Driffter menggunakan XBee dalam **transparent mode (AT mode)** — mode default dari pabrik. Dalam mode ini, XBee bertindak seperti kabel serial wireless:

- Setiap byte yang masuk ke TX pin akan keluar di RX pin di sisi pasangan — **tanpa packet framing, tanpa addressing di data stream**
- XBee secara internal melakukan packetization: buffering byte, membentuk 802.15.4 frame, transmisi, ACK di tingkat PHY
- Jika transmisi gagal (no ACK), XBee retry hingga konfigurasi (default 3x)
- Latency tambahan: ~3-5ms (buffering + CSMA/CA channel access + transmission time)

Mode API (yang tidak kita gunakan) akan memberikan kontrol lebih: addressing, RSSI monitoring, remote configuration — tetapi menambah kompleksitas parsing di Arduino.

**XBee dalam ArduSimple kit**: Sudah dikonfigurasi dari pabrik — PAN ID dan Destination Address sudah dipasangkan. Cukup colok dan nyala — tidak perlu XCTU untuk initial setup. Jika komunikasi gagal, baru periksa XCTU.

---

## 12. Sistem Koordinat & Geometri Bola

### 12.1 WGS84 — Referensi Standar GPS

WGS84 (World Geodetic System 1984) adalah datum geodetik yang digunakan oleh GPS. Mendefinisikan ellipsoid referensi dengan parameter:

- Semi-major axis: `a = 6,378,137.0 m` (radius ekuatorial)
- Reciprocal flattening: `1/f = 298.257223563`
- Flattening: `f = 1/298.257223563 ≈ 0.00335281066`
- Semi-minor axis: `b = a(1 − f) = 6,356,752.3142 m` (radius polar)
- Eccentricity squared: `e² = 2f − f² = 0.00669437999014`

Ellipsoid adalah aproksimasi permukaan bumi — bukan bola sempurna karena rotasi bumi menyebabkan equatorial bulge (radius ekuator 21.4 km lebih besar dari radius polar). Di ekuator, perbedaan ellipsoid vs sphere rata-rata adalah ~0.3% — kecil tapi signifikan untuk perhitungan jarak presisi.

### 12.2 Koordinat Geodetik (Lat/Lon/Height)

Latitude geodetik (φ, yang digunakan di GPS, NMEA, dan dashboard kita) didefinisikan sebagai sudut antara normal ellipsoid (bukan vektor dari pusat bumi!) dengan bidang ekuator. Ini berbeda dengan latitude geosentrik (ψ, dari pusat bumi) — perbedaan maksimum ~0.2° (sekitar 22 km di permukaan) pada latitude 45°. Rumus konversi:

```
tan(ψ) = (b²/a²) × tan(φ)
```

Dengan `b²/a² = (1−f)² ≈ 0.9933`.

### 12.3 Konversi NMEA DDMM.MMMMMM → Decimal

Format latitude NMEA: `DDMM.MMMMMM` — D = degrees (2 digit), M = minutes (6+ digit desimal).

Untuk `0746.1055180,S`:
```
Degrees = floor(7446.1055180 / 100) = 7
Minutes = 746.1055180 − 7×100 = 46.1055180
Decimal = ±(7 + 46.1055180/60) = −7.76842530

Mengapa negatif? S = South → latitude negatif di belahan selatan
```

Untuk longitude `11023.6272104,E`:
```
Degrees = floor(11023.6272104 / 100) = 110
Minutes = 23.6272104
Decimal = +110.39378684
```

Resolusi format: 6 digit desimal minutes → resolusi latitude = (0.000001/60)° × 111,319m/° ≈ 1.85 cm — cukup untuk akurasi RTK (1-2cm) namun berada di batas bawah.

### 12.4 Haversine Formula — Jarak di Permukaan Bola

Untuk menghitung jarak antara dua titik di permukaan Bumi dengan aproksimasi bola (R ≈ 6,371 km):

```
a = sin²(Δφ/2) + cos(φ1)·cos(φ2)·sin²(Δλ/2)
c = 2 · atan2(√a, √(1−a))
d = R · c
```

di mana Δφ = φ2 − φ1, Δλ = λ2 − λ1 (dalam radian).

**Mengapa Haversine, bukan spherical law of cosines?** Untuk jarak pendek (<<1 km), Δφ dan Δλ sangat kecil → cos(Δφ) ≈ 1 − Δφ²/2 → spherical law of cosines menghitung `1 − 1 = 0` → presisi numerik hilang (cancellation). Haversine tidak mengalami masalah ini: `sin²(x/2)` untuk x kecil ≈ x²/4 — tetap representable di floating-point.

Untuk akurasi lebih tinggi (sub-meter) atau jarak >100km: gunakan Vincenty formula (menyelesaikan ellipsoid tanpa aproksimasi bola) — iterasi hingga konvergen, akurasi ±0.5mm. Namun untuk Coastal Driffter (baseline <5km), Haversine dengan R=6,371,000m memiliki error <1 cm — lebih dari cukup.

### 12.5 Altitude — MSL, Ellipsoid, Geoid

GNSS receiver memberikan beberapa definisi "ketinggian" yang berbeda:

- **Ellipsoid height (h)**: Jarak sepanjang normal ellipsoid dari permukaan ellipsoid WGS84 ke antena. Inilah yang dihitung langsung dari trilaterasi GNSS.

- **Geoid undulation (N)**: Jarak dari ellipsoid ke geoid (permukaan ekipotensial gravitasi yang mendekati mean sea level). Bervariasi global: −105m (selatan India) hingga +85m (sekitar Papua). Di Indonesia: N ≈ +15m hingga +25m.

- **MSL height (H = h − N)**: Ketinggian di atas mean sea level — standar untuk navigasi dan display.

- **UBX-NAV-PVT**: menyediakan `hMSL` (offset 36, mm MSL) dan `height` (offset 32, mm ellipsoid)

Yang digunakan dashboard: **hMSL** (mean sea level), sama seperti altitude GPS di aplikasi ponsel. Jika hanya NMEA GGA yang tersedia, altitude MSL dari field 9.

---

## 13. Korelasi Penuh di Coastal Driffter

### 13.1 Diagram End-to-End

```
                       SEGMEN SPACE (trilaterasi posisi)
     ┌─────────────────────────────────────────────────────────────┐
     │ GPS L1/L2    GLONASS L1/L2    Galileo E1/E5b    BeiDou B1I │
     │  4+ satellites tracked simultaneously by each GNSS receiver │
     └────────┬──────────────────────────────────────┬─────────────┘
              │ carrier phase + pseudorange (L1 λ=19cm, L2 λ=24cm)
              ▼                                      ▼
    ┌──────────────────┐                  ┌──────────────────┐
    │ BASE GNSS        │                  │ ROVER GNSS       │
    │ ZED-F9P (UART1)  │                  │ ZED-F9P (UART1)  │
    │ TMODE3 Survey-In │                  │ Rover mode       │
    │                  │                  │                  │
    │ Output:          │                  │ Output:          │
    │ • UBX-NAV-PVT    │   XBee wireless  │ • UBX-NAV-PVT    │
    │ • RTCM3 MSM4-7   │─────────────────▶│ • UBX-NAV-PVT    │
    │ • RTCM3 1005     │  RTCM3 koreksi   │   (corrected!)   │
    │                  │  (byte-by-byte)  │ • UBX-RELPOSNED  │
    │                  │◀─────────────────│ • NMEA GGA/RMC   │
    └──────┬───────────┘  Rover data     └────────┬─────────┘
           │ UART 38400                          │ UART 38400
           ▼                                      ▼
   gnssSerial (pin 4,5)                 radioSerial (pin 2,8)
   SoftwareSerial ATmega328P            via XBee @ 9600
           │                                      │
           ▼                                      ▼
   ┌─────────────────────────────────────────────────────────────┐
   │                    ARDUINO BASE UNO                        │
   │                                                            │
   │  loop():                                                   │
   │    1. gnssSerial.listen()                                  │
   │       • Base: UBX parser (Fletcher checksum) → base_data   │
   │         (lat_1e7, lon_1e7, fix, hAcc_mm, alt_mm)           │
   │       • Relay ALL bytes → radioSerial (RTCM3 forwarding)   │
   │                                                            │
   │    2. radioSerial.listen()                                 │
   │       • Rover: UBX parser → rover_data                     │
   │         (lat_1e7, lon_1e7, alt_mm, fix, hAcc_mm, gSpeed,   │
   │          headMot, relN_cm, relE_cm)                        │
   │                                                            │
   │    3. Every 200ms: sendJson()                              │
   │       Format: {"base":{lat,lon,alt},"rover":{lat,lon,alt,  │
   │                fix,hAcc_cm,speed_kmh,heading,...,          │
   │                relN_m,relE_m}}                              │
   └────────────────────────┬───────────────────────────────────┘
                            │ USB Serial 115200 bps
                            ▼
   ┌─────────────────────────────────────────────────────────────┐
   │                 DASHBOARD (Chrome/Edge)                     │
   │                 map_dashboard.html                          │
   │                                                            │
   │  Web Serial API read:                                      │
   │    • If JSON line: updateDashboard(data)                   │
   │    • If $GNRMC: parseNmeaRmc() → buildNmeaData()           │
   │    • If $GNGGA: parseNmeaGga() → store fix quality + hDOP  │
   │                                                            │
   │  updateDashboard():                                        │
   │    • basePos → Leaflet marker (📌 Base Station)            │
   │    • roverPos → Leaflet marker (🚤 Coastal Drifter)        │
   │    • pathPolyline.addLatLng(roverPos) → trail line         │
   │    • baselineLine([basePos, roverPos]) → dashed line       │
   │    • Haversine(basePos, roverPos) → baseline distance      │
   │    • Haversine(lastPos, roverPos) → accumulate total dist  │
   │    • carrSoln/fix → badge (RTK FIX/FLOAT/SINGLE/NO FIX)   │
   │    • Export: CSV, GPX, KML                                 │
   └─────────────────────────────────────────────────────────────┘
```

### 13.2 Mapping Istilah ke Implementasi

| Istilah Teknis | Di Mana Digunakan | File | Baris/Bagian |
|---|---|---|---|
| Trilaterasi (4 satelit min) | Receiver internal | — | Proses otomatis di ZED-F9P |
| NMEA $GNRMC parsing | Dashboard — posisi + speed + heading | `map_dashboard.html` | `parseNmeaRmc()` |
| NMEA DDMM.MMMMM → decimal | Dashboard — konversi koordinat | `map_dashboard.html` | `parseNmeaDeg()` |
| NMEA $GNGGA fix quality | Dashboard — badge RTK status | `map_dashboard.html` | `parseNmeaGga()` field 6-8 |
| UBX SYNC 0xB5 0x62 | Arduino Base — frame detection | `Arduino_Base.ino` | `processUbxByte()` |
| Fletcher checksum | Arduino Base — verifikasi packet | `Arduino_Base.ino` | CK_A + CK_B in state machine |
| UBX-NAV-PVT parse | Arduino Base — posisi + fix + hAcc | `Arduino_Base.ino` | `parseNavPvt()` |
| carrSoln bits 6:7 | Arduino Base — RTK FIX/FLOAT detection | `Arduino_Base.ino` | `p[21] >> 6 & 0x03` |
| UBX-NAV-RELPOSNED | Arduino Base — baseline vector | `Arduino_Base.ino` | `parseRelposned()` |
| RTCM3 relay | Arduino Base — forwarding Base→Rover | `Arduino_Base.ino` | `radioSerial.write(b)` |
| TMODE3 Survey-In | Base GNSS — posisi otomatis | u-center config | TMODE3 menu |
| SoftwareSerial.listen() | Arduino Base — RX switching | `Arduino_Base.ino` | `gnssSerial.listen()` / `radioSerial.listen()` |
| JSON merge (base+rover) | Arduino Base — output ke PC | `Arduino_Base.ino` | `sendJson()` |
| Haversine formula | Dashboard — jarak baseline + total track | `map_dashboard.html` | `calculateDistance()` |
| Web Serial API | Dashboard — komunikasi PC↔Arduino | `map_dashboard.html` | `navigator.serial.requestPort()` |
| Leaflet.js markers | Dashboard — visualisasi peta | `map_dashboard.html` | `L.marker()`, `L.polyline()` |
| CSV/GPX/KML export | Dashboard — logging | `map_dashboard.html` | `downloadFile()` |
| DOP (HDOP/PDOP/VDOP) | Receiver internal + NMEA $GNGSA | — | Diinterpretasikan di Bab 9 |
| WGS84 ellipsoid | GNSS internal coordinate frame | — | Semua koordinat dalam WGS84 |
| Cycle slip | Degradasi RTK FIX→FLOAT | — | Terlihat di dashboard sebagai perubahan fix status |
| Multipath over water | Degradasi sinyal di coastal | — | Penyebab utama fix tidak stabil |

### 13.3 Aliran Waktu Kejadian

```
t=0     : Nyalakan Base GNSS + Arduino Base + XBee
t=0     : Nyalakan Rover GNSS + XBee + baterai
t=0-5s  : Base GNSS boot → mode Survey-In (LED berkedip)
t=5-30s : Rover GNSS boot → first fix, mode SINGLE (fix=1)
t=5-30s : Arduino Base boot → mulai loop, radioSerial mendengarkan Rover
t=30s   : Rover mulai kirim NMEA (RMC + GGA) via XBee → Dashboard mode NMEA bisa dibuka @ 38400
t=60-120s: Base Survey-In selesai → mulai output RTCM3 + UBX-NAV-PVT
t=60-120s: Arduino mulai menerima UBX dari Base → update base.lat/base.lon
t=61-125s: Rover menerima RTCM3 via XBee → mode RTK FLOAT (fix=5)
t=65-150s: Rover integer ambiguity resolved → mode RTK FIX (fix=4)
t=65+    : Sistem stabil — Arduino mengirim JSON dengan:
          - base: posisi dari UBX-NAV-PVT Base
          - rover: posisi dari UBX-NAV-PVT Rover (RTK FIX)
          - rover.relN_m, rover.relE_m: dari UBX-NAV-RELPOSNED
t=65+    : Dashboard mode JSON @ 115200 — live tracking 2 titik + baseline
t=∞      : Selama Base tetap stasioner dan XBee dalam jangkauan,
          sistem terus beroperasi dengan akurasi 1-2 cm + 1 ppm
```

### 13.4 Perbandingan Mode Operasi

| | Mode NMEA (Rover-Only) | Mode JSON (Full System) |
|---|---|---|
| **Baud Dashboard** | 38400 | 115200 |
| **Protokol** | NMEA ASCII ($GNRMC, $GNGGA) | JSON via UBX binary parsing |
| **Arduino Base** | Program passthrough sederhana | `Arduino_Base.ino` firmware penuh |
| **Base GNSS** | Tidak digunakan | TMODE3 Survey-In → RTCM3 + UBX |
| **Rover GNSS** | Output NMEA | Output UBX (+ NMEA fallback) |
| **Akurasi Posisi** | 3-15m (autonomous) | 1-2 cm (RTK FIX) |
| **Fix Quality** | 1 (SINGLE) permanent | 4 (FIX) atau 5 (FLOAT) |
| **Baseline Info** | Tidak tersedia | RELPOSNED: North/East distance to Base |
| **Map Dashboard** | 1 titik (Rover) | 2 titik (Base 📌 + Rover 🚤) + baseline line |
| **hAcc** | Dari HDOP (estimasi kasar) | Dari UBX hAcc (mm, presisi) |
| **Cycle Slip** | Tidak relevan | Terlihat di dashboard (FIX→FLOAT transition) |
| **Gunakan Saat** | Initial test, cek hardware | Operasional tracking, deployment lapangan |

---

*Coastal Driffter — Glosarium & Referensi Teknis, revisi 2026.*
