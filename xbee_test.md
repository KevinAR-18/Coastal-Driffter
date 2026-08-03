# XBee Communication Test Guide

Panduan pengujian komunikasi XBee Base ↔ XBee Rover menggunakan Arduino UNO.

---

## 1. Tujuan

Verifikasi bahwa XBee Base dan XBee Rover dapat berkomunikasi via radio 2.4GHz **sebelum** seluruh sistem Coastal Driffter diaktifkan. Ini adalah fondasi — kalau XBee mati, gak mungkin dapat RTK FIX dan posisi Rover.

---

## 2. Hardware

| Qty | Komponen | Peran |
|---|---|---|
| 2x | Arduino UNO | 1 Base (Sender) + 1 Rover (Receiver) |
| 2x | XBee modul (dari ArduSimple kit) | Radio link |
| — | Kabel jumper male-female | Wiring |
| 1x | Power bank / baterai 5V | Power Rover (setelah upload) |
| 1x | Kabel USB | Upload + Serial Monitor Base |

> ⚠️ Tidak ada GNSS yang dicolok — ini murni test XBee.

---

## 3. Wiring

### Kedua sisi — IDENTIK

| XBee Pin | Arduino UNO Pin | Keterangan |
|---|---|---|
| TX (keluar data) | **Pin 2** | SoftwareSerial RX |
| RX (masuk data) | **Pin 8** | SoftwareSerial TX |
| VCC | **3.3V** | ⚠️ Jangan 5V — XBee bisa rusak |
| GND | **GND** | |

---

## 4. Upload Firmware

### Langkah 1: Arduino Base (Sender)

1. Buka file `xbee_test_sender.ino` di Arduino IDE
2. Pilih **Tools → Board → Arduino UNO**
3. Pilih **Tools → Port → COM port Arduino Base**
4. Upload

### Langkah 2: Arduino Rover (Receiver)

1. **Pindahkan kabel USB** ke Arduino Rover
2. Buka file `xbee_test_receiver.ino` di Arduino IDE
3. Port otomatis berubah — pastikan terpilih COM port Rover
4. Upload

### Langkah 3: Siapkan Rover untuk Test

1. **Cabut USB dari Arduino Rover**
2. Sambung XBee Rover ke Arduino Rover (sesuai wiring di atas)
3. Power Arduino Rover pakai **power bank / baterai** ke:
   - DC barrel jack, ATAU
   - Pin VIN + GND
4. Perhatikan LED built-in (pin 13):
   - 🔵 **Berkedip 2x** → program berjalan normal
   - 🔴 **Tidak berkedip** → cek power / upload ulang

> Kenapa harus cabut USB di Rover? Pin 0 dan 1 dipakai USB-to-Serial — kalau USB masih nancep, timing SoftwareSerial bisa terganggu dan data dari XBee gak kebaca.

---

## 5. Menjalankan Test

### Sisi Base

1. USB tetap dicolok ke Arduino Base (dan ke PC)
2. Pasang XBee Base ke Arduino Base sesuai wiring
3. Di Arduino IDE: **Tools → Port** → pilih COM port Base
4. Buka **Serial Monitor** (Ctrl+Shift+M)
5. Set baud rate: **9600**
6. Harusnya muncul:
   ```
   === XBEE SENDER (BASE) ===
   Kirim 'H' setiap 1 detik via XBee...

   HHH...
   ```

### Sisi Rover

Lihat LED built-in (pin 13) Arduino Rover:

| LED Rover | Artinya | Aksi |
|---|---|---|
| 🔵 **Berkedip tiap ~1 detik** | ✅ **XBee komunikasi OK!** | Siap lanjut ke full system |
| 🔵 **Berkedip 2x (boot) lalu MATI** | ❌ XBee tidak terhubung | Lanjut ke Troubleshooting |
| 🔵 **Berkedip random/jarang** | ⚠️ Packet loss | Coba dekatkan kedua XBee, jangan ada penghalang |
| 🔴 **Tidak berkedip sama sekali** | ❌ Program tidak jalan / power mati | Cek power, upload ulang |

---

## 6. Troubleshooting

### LED Rover tidak berkedip

| Cek | Pertanyaan | Solusi |
|---|---|---|
| Power Rover | Arduino Rover nyala? Ada LED hijau ON? | Cabut-pasang power bank / baterai. Cek kabel. |
| Upload Rover | LED boot (2x kedip) muncul? | Upload ulang `xbee_receiver.ino`, pastikan Board=UNO, Port benar. |
| XBee menyala? | Ada LED merah kecil di modul XBee? | Cek kabel 3.3V dan GND. XBee butuh ~50mA — Arduino 3.3V pin bisa supply ini. |
| Wiring benar? | TX→pin 2, RX→pin 8? | Cek ulang. Pin 2 = RX Arduino (terima dari XBee TX). Pin 8 = TX Arduino (kirim ke XBee RX). |
| XBee paired? | PAN ID dan Destination Address sama? | Lihat Section 7 (XCTU) |

### XBee Terlihat Nyala Tapi Tetap Tidak Komunikasi

**Ini hampir pasti masalah pairing.** XBee harus di-set supaya satu sama lain saling kenal. Dari ArduSimple kit, XBee sudah di-pair dari pabrik — tapi tidak ada salahnya verifikasi.

---

## 7. XBee Pairing via XCTU

### Install XCTU

1. Download dari: https://www.digi.com/products/embedded-systems/digi-xbee/digi-xbee-tools/xctu
2. Install dan buka

### Cek XBee Satu Per Satu

1. Colok **XBee Base** ke Arduino, lalu Arduino ke PC via USB
2. Upload sketch **kosong** (atau biarkan bootloader). XBee akan terhubung ke XBee USB secara passthrough
3. Tapi lebih mudah: pakai **USB Explorer / XBee Adapter** (kalau punya). Kalau tidak:
   - Buka XCTU → **Add Radio Module** → pilih COM port Arduino → baud 9600 → Finish
   - XCTU akan membaca konfigurasi XBee
4. Catat **PAN ID** (default: 333), **DH + DL** (Destination Address), **CH** (Channel)

5. Ulangi untuk **XBee Rover** (colok ke Arduino Rover → USB → XCTU)

### Setting yang Harus SAMA

| Parameter | XBee Base | XBee Rover | Harus? |
|---|---|---|---|
| **PAN ID** | 333 (contoh) | 333 | **Harus SAMA** |
| **CH** (Channel) | C | C | **Harus SAMA** |
| **DH + DL** (Destination) | MAC Rover | MAC Base | **Harus menunjuk ke pasangan** |
| **BD** (Baud Rate) | 9600 | 9600 | Harus sama dengan Arduino |
| **CE** (Coordinator) | 1 (Coordinator) | 0 (Router/End) | Tidak wajib, tapi disarankan |

> Kalau kamu punya 2 port USB (atau 2 laptop), bisa buka 2 instance XCTU sekaligus — lebih mudah.

### Reset ke Default Jika Perlu

Di XCTU: pilih modul → **Update** → pilih firmware terbaru → centang "Erase current settings" → Update. Kemudian set ulang PAN ID, Destination, Baud.

---

## 8. Setelah Test Berhasil

Kalau LED Rover berkedip tiap detik, XBee **sudah siap**. Langkah selanjutnya:

1. Kembalikan wiring ke konfigurasi **full system**:
   - Base: GNSS pin 4,5 + XBee pin 2,8 + USB ke PC
   - Rover: GNSS langsung ke XBee via shield + baterai
2. Konfigurasi GNSS di u-center (lihat `guide.md`)
3. Upload `Arduino_Base.ino`
4. Buka dashboard pilih **115200**, connect

---

*XBee Communication Test Guide — Coastal Driffter, 2026.*
