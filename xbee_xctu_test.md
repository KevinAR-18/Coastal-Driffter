# XCTU — Panduan Uji XBee SX

Panduan praktis menguji komunikasi XBee SX Base ↔ Rover lewat XCTU, mulai
dari discover devices hingga range test.

Uji ini **melengkapi**, bukan menggantikan, `xbee_test.md` (uji via Arduino).
Kelebihan XCTU: mengungkap parameter yang tidak bisa diakses dari Arduino.

---

## 1. Prasyarat

| Item | Wajib? | Catatan |
|---|---|---|
| **USB adapter XBee** | **Ya** | XBee Explorer, Digi XBIB, atau adapter pihak ketiga. Tanpa ini XCTU tidak bisa mengakses modul. |
| XCTU terinstal | **Ya** | Unduh: https://www.digi.com/products/embedded-systems/digi-xbee/digi-xbee-tools/xctu |
| 1 XBee terpasang di Arduino | Opsional | Untuk uji teks dua arah dengan XCTU Console di satu sisi + Arduino di sisi lain |

Untuk uji Console dua arah keduanya lewat XCTU: perlu **2 adapter** + **2 PC**
(atau 2 instance XCTU di 1 PC dengan 2 port COM berbeda).

---

## 2. Discover Devices

**Ini sekaligus mengungkap baud rate pabrik XBee tanpa menebak-nebak.**
XCTU mencoba semua baud rate yang dicentang secara berurutan sampai modul
merespons `AT` dengan `OK`.

### Langkah

1. **Cabut XBee dari socket simpleRTK2B**, pasang ke USB adapter.
2. Colok adapter ke PC.
3. Buka XCTU.
4. Klik ikon **Add devices** / **Discover radio modules** (ikon kaca pembesar
   di toolbar kiri atas, atau ikon XBee dengan tanda `+`).
5. Pilih **port COM** adapter (cek di Device Manager kalau ragu).
6. Centang **semua baud rate** di daftar parameter pencarian — ini memakan
   waktu lebih lama tapi menghilangkan variabel "baud tidak diketahui".
7. Klik **Finish** / **Start Discovery**.

### Hasil yang diharapkan

```
[COM3] Radio module found:
  Device type: XBee SX
  Firmware version: 80xx
  MAC: 0013A200XXXXXXXX
```

### Kalau tidak ditemukan

| Penyebab | Cek |
|---|---|
| Baud pabrik di luar yang dicentang | Centang *semua* opsi baud, ulangi |
| Modul dalam API mode (`AP ≠ 0`) | Centang "API mode" di parameter pencarian |
| Adapter rusak / tidak tersambung | LED adapter menyala? Cek Device Manager |
| Modul brick | Lihat §9 |

Setelah satu modul ditemukan, **segera simpan profilnya**: klik kanan modul →
**Export Profile** → simpan sebagai `xbee_base_original.xpro` atau
`xbee_rover_original.xpro`. Ini asuransi — kalau ada setting yang keliru,
kamu bisa restore lewat **Import Profile**.

Ulangi langkah 1-7 untuk **modul kedua**.

---

## 3. Baca & Verifikasi Konfigurasi

Setelah modul ditemukan, XCTU menampilkan daftar parameter di tab
**Configuration**. Yang perlu diverifikasi:

| Parameter | Cek | Prioritas |
|---|---|---|
| **AP** | Harus `0` (Transparent) | 🔴 **Kritis** — kalau bukan 0, RTCM3 dan UBX rusak oleh frame Digi |
| **BD** | Catat nilainya — ini baud pabrik | 🟡 Perlu diketahui untuk dicocokkan dengan ZED-F9P UART2 |
| **ID** | Harus **sama** di kedua modul | 🔴 Kalau beda, kedua XBee ada di jaringan berbeda |
| **DH + DL** | Saling menunjuk ke MAC pasangan | 🟡 Kalau broadcast (DH=0, DL=FFFF) juga bisa, tapi tanpa ACK |

Bandrol parameter yang sudah cocok — untuk sekarang tidak ada yang diubah dulu.
**Baca saja.** Verifikasi bahwa kedua modul terlihat identik untuk `ID`.

---

## 4. Uji Console — Kirim/Terima Teks

Console Mode di XCTU memungkinkan kamu mengetik teks dan mengirimkannya lewat
radio — persis seperti chat terminal. Ini adalah **cara tercepat untuk
memverifikasi bahwa kedua XBee benar-benar saling terhubung**.

### Skenario A: Dua Adaptor, Dua PC / Dua Instance XCTU

1. Buka tab **Console** di kedua modul.
2. Ketik teks di Console sisi Base, akhiri dengan Enter.
3. Teks muncul di Console sisi Rover — dan sebaliknya.

### Skenario B: Satu Adaptor XCTU + Satu Arduino

Ini skenario paling realistis kalau kamu cuma punya satu adapter:

1. XBee Base → adapter → XCTU Console.
2. XBee Rover → Arduino (wiring sesuai `xbee_test.md` §3), upload
   `xbee_test_receiver.ino`.
3. Buka **Serial Monitor Arduino** (Rover, baud 115200).
4. Di XCTU Console (Base), klik **Open**, ketik `PING`, kirim.
5. Kalau Serial Monitor Rover menampilkan `PING` lalu membalas `PONG`,
   komunikasi **dua arah** sudah berfungsi.

> Untuk mengirim teks dengan newline di XCTU Console: ketik teks lalu tekan
> Enter, atau gunakan tombol **Send packet** dan pilih format yang sesuai.
> Tab **Hex** berguna untuk mengirim byte non-ASCII.

Console juga mengonfirmasi `AP = 0`: kalau yang muncul di sisi penerima adalah
teks yang kamu ketik, berarti transparent mode aktif. Kalau yang muncul adalah
byte acak, kemungkinan `AP ≠ 0`.

---

## 5. Range Test — RSSI & Packet Loss

Range Test mengirim paket berulang dari satu XBee ke XBee lain dan mengukur
berapa persen yang berhasil diterima. **Ini yang kamu butuhkan di bring-up
tahap 4-6** (`guide.md` §8) — untuk membedakan kegagalan karena sinyal lemah
vs kegagalan karena bandwidth saturasi.

### Cara

1. Pastikan **kedua XBee menyala** — minimal satu tersambung ke XCTU, yang
   satunya cukup terpasang di simpleRTK2B dan menyala.
2. Di XCTU, pilih modul yang tersambung (biasanya Base).
3. Klik ikon **Range Test** di toolbar, atau dari menu **Tools → Range Test**.
4. XCTU akan mencari `remote` — modul pasangan dalam jaringan yang sama.
5. Pilih remote, klik **Start**.

### Membaca hasil

| Metrik | Arti |
|---|---|
| **RSSI (Received Signal Strength Indicator)** | Semakin dekat ke 0, semakin kuat. Di bawah -90 dBm = ambang batas. |
| **Packet Success Rate** | Persentase paket yang sampai. < 80% = link tidak sehat. |
| **Packets Sent / Received** | Hitungan mentah; RX < TX = ada yang hilang |

### Interpretasi

| RSSI | Packet Success | Diagnosa |
|---|---|---|
| > -70 dBm | > 95% | Link sehat. Kalau RTK gagal, masalahnya **bukan radio**. |
| -70 s/d -85 dBm | 80-95% | Link mencukupi. Packet loss rendah masih bisa ditoleransi UBX (checksum akan menolak frame rusak), tapi RTCM3 bisa bolong. |
| -85 s/d -95 dBm | 50-80% | Link marjinal. Butuh antenna lebih tinggi, power lebih besar, atau turunkan RF rate. |
| < -95 dBm | < 50% | Link tidak cukup untuk RTK. |

Ulangi range test di **jarak bertahap** sesuai bring-up (`guide.md` §8):
50 m → 500 m → 2 km → 5 km. Catat RSSI dan packet success di setiap titik —
data ini sekaligus menjadi hasil range test yang bisa dilaporkan.

> ⚠️ Range Test bergantung pada firmware XBee mendukung **loopback**. Pada
> sebagian varian, fitur ini tidak tersedia. Kalau tombol Range Test abu-abu
> atau remote tidak ditemukan, akses XCTU Console di kedua sisi sebagai
> alternatif.

---

## 6. Ubah Parameter

Setelah selesai verifikasi dan kamu yakin dengan nilai target, **baru** tulis
perubahan ke modul.

### Urutan yang aman

1. Klik **Read** (ikon panah bawah) — pastikan tampilan parameter sesuai
   dengan yang ada di modul, bukan cache.
2. Ubah parameter satu per satu **dengan jeda Write** — jangan ubah 5
   parameter sekaligus lalu Write. Kalau gagal, kamu tahu persis penyebabnya.
3. Klik **Write** (ikon pensil) setelah tiap perubahan.
4. Klik **Read** lagi untuk verifikasi bahwa nilai benar-benar tersimpan.

### Parameter yang perlu diubah

| Parameter | Dari | Ke | Setelah |
|---|---|---|---|
| **BD** | (nilai pabrik) | `19200` | Setelah kedua modul selesai |
| **BR** (RF Data Rate) | (nilai pabrik) | Long range | Sebelum uji >1 km |

Rekomendasi: **ubah `BD` terakhir**, karena XCTU harus bicara di baud yang
sama dengan modul. Begitu `BD` berubah, XCTU harus reconnect — kalau kamu
centang baud 19200 di Discover, reconnect otomatis.

Kalau modul berhenti merespons setelah Write: jangan panik, lihat §9.

---

## 7. Verifikasi Perubahan Bertahan

Setelah semua parameter di-write dan diverifikasi:

1. **Cabut adapter dari USB** — matikan daya ke XBee.
2. Colok lagi — XCTU reconnect.
3. Klik **Read**.
4. Pastikan semua nilai sesuai yang kamu tulis.

Ini memverifikasi bahwa parameter tersimpan di **non-volatile memory** — kalau
hanya bertahan saat daya menyala tapi hilang setelah restart, tombol Write
tidak benar-benar menyimpan.

---

## 8. Simpan Profil untuk Repo

Setelah kedua modul selesai dikonfigurasi, simpan profil final:

- Di XCTU: klik kanan modul → **Export Profile**.
- Simpan sebagai `config/xbee_base_final.xpro` dan
  `config/xbee_rover_final.xpro` di repo.

Profil `.xpro` berisi semua parameter modul — bisa dipakai untuk
restore cepat, atau untuk menginspeksi konfigurasi tanpa modul fisik.

---

## 9. Troubleshooting

### Modul tidak ditemukan saat Discover

| Penyebab | Gejala | Solusi |
|---|---|---|
| Baud tidak cocok | Discover selesai, tidak ada hasil | Centang semua baud (termasuk 115200) + API mode |
| Modul brick / firmware korup | Tidak ada respons di baud mana pun | Lihat §10 |
| Adapter tidak tersambung | LED adapter tidak menyala | Tukar kabel USB, cek Device Manager |

### Modul ditemukan tapi Console menampilkan sampah

| Penyebab | Solusi |
|---|---|
| **`AP ≠ 0`** (API mode) | Ganti tampilan Console ke **Hex**, atau ubah `AP` ke `0` dan Write |
| Baud salah | Modul mungkin ditemukan di baud yang berbeda dari yang sekarang digunakan Console. Cek `BD` di Configuration, pastikan Console terbuka di baud yang sama. |

### Remote tidak muncul di Range Test

| Penyebab | Solusi |
|---|---|
| XBee pasangan mati / di luar jangkauan | Nyalakan pasangan, dekatkan |
| `ID` (PAN ID) tidak sama | Verifikasi `ID` kedua modul identik |

### Tidak bisa menulis perubahan / "Write failed"

| Penyebab | Gejala | Solusi |
|---|---|---|
| XCTU koneksi terputus setelah mengubah `BD` | Modul pindah baud, XCTU belum reconnect | Discover ulang dengan centang baud target |
| Modul dalam mode Command (`+++`) tidak selesai | XCTU timeout | Cabut-colok adapter, coba Write ulang |

---

## 10. Recovery Modul Brick

Kalau modul benar-benar tidak merespons di baud mana pun:

1. Buka XCTU → pilih modul (kalau masih terdaftar) → klik **Update**.
2. Pilih firmware terbaru yang cocok — pastikan **tipe modul** benar
   (XBee SX 868 vs SX 900 firmware berbeda).
3. Centang **"Erase current settings"**.
4. Klik **Update** — XCTU akan mem-flash firmware pabrik.
5. Setelah selesai, Discover ulang.
6. Restore profil dari backup, atau atur ulang parameter secara manual.

> ⚠️ Hati-hati memilih firmware: SX 868 dan SX 900 adalah hardware berbeda.
> Firmware yang salah **tidak akan merusak secara fisik**, tapi harus
> di-flash ulang dengan firmware yang benar.

---

## 11. Setelah Test XCTU Selesai

1. Kedua XBee sudah di `BD = 19200`, `AP = 0`, `ID` cocok, dan `BR` long
   range — **simpan profil final** (§8).
2. Kembalikan XBee ke socket simpleRTK2B masing-masing.
3. Verifikasi link dengan `xbee_test.md` di baud **19200**.
4. Lanjut ke `guide.md` §3 (konfigurasi u-center Base).
5. **Sebelum ke lapangan**: pastikan §6 tabel baud — kelima titik 19200 —
   sudah terpenuhi semua.

---

*XCTU Test Guide — Coastal Driffter, 2026.*
