# Desain Smart Parking

Tugas Akhir IoT. Tim 4 orang. Deadline sekitar 8 Desember 2026.

## Ringkasan

Maket parkir 1 slot dengan 1 gerbang untuk masuk dan keluar. ESP32 membaca sensor slot (FC-51) dan sensor gerbang (IR break beam). ESP32 menggerakkan plang (servo) dan menampilkan status di LCD, LED, dan buzzer. Status dan kontrol plang tersedia secara remote lewat MQTT (EMQX Serverless) dan dashboard Node-RED.

## Hardware

| Komponen | Fungsi |
|---|---|
| ESP32 | Mikrokontroler + WiFi |
| FC-51 | Deteksi mobil di slot parkir |
| IR break beam | Dipasang segaris tepat di bawah plang. Berfungsi sebagai pemicu sekaligus pengaman plang |
| Servo | Plang parkir. 0° = tutup, 90° = buka |
| LCD 16x2 I2C | Tampilan status |
| LED hijau + merah | Indikator slot |
| Buzzer | Bunyi notifikasi |

### Catu daya

- Servo dicatu dari adaptor 5V 2A terpisah.
- ESP32 dicatu lewat USB.
- GND adaptor dan GND ESP32 disatukan.
- Pasang kapasitor 470–1000µF di jalur power servo untuk meredam lonjakan arus. Tanpa ini, ESP32 bisa brownout dan reset.

### Pemetaan pin

| Komponen | Pin ESP32 | Catatan |
|---|---|---|
| FC-51 OUT | GPIO 27 | VCC 3.3V |
| Break beam receiver | GPIO 26 | `INPUT_PULLUP`, VCC 3.3V |
| Servo signal | GPIO 13 | Power dari adaptor 5V |
| LCD SDA / SCL | GPIO 21 / 22 | VCC 5V. Pull-up modul ke 5V biasanya masih aman. Pakai level shifter untuk opsi paling aman |
| LED hijau | GPIO 25 | Resistor 220Ω |
| LED merah | GPIO 33 | Resistor 220Ω |
| Buzzer | GPIO 32 | Diasumsikan buzzer aktif. Jika pasif, pakai `tone()` |

## Logika

### Pembacaan sensor

- Status slot dari FC-51 dianggap berubah hanya jika stabil selama 1 detik (debounce).

### Mode Auto (default)

| Kondisi | Aksi |
|---|---|
| Beam terhalang dan slot kosong | Plang dibuka. Berlaku untuk mobil masuk maupun keluar |
| Beam terhalang dan slot terisi | Akses ditolak. Buzzer bunyi 3 kali, LCD menampilkan `Parkir Penuh` |
| Beam bersih selama 3 detik | Plang ditutup |

Arah mobil tidak perlu dideteksi. Dengan 1 slot, beam terhalang saat slot terisi pasti berarti ada mobil baru yang datang.

Keterbatasan yang diterima: jika mobil kedua datang sebelum mobil pertama sampai di slot, plang tetap terbuka untuk mobil kedua.

### Mode Manual (dari Node-RED)

- Beam tidak otomatis membuka plang.
- Plang hanya bergerak lewat tombol Buka/Tutup di dashboard.
- Perintah Tutup ditunda selama beam terhalang. Pengaman ini aktif di semua mode.
- LCD, LED, dan status slot tetap diperbarui.

Di mode Auto, perintah tombol Buka/Tutup diabaikan oleh ESP32.

### Saat offline

- Jika WiFi atau MQTT putus, ESP32 kembali ke mode Auto secara lokal.
- Sensor, plang, LCD, LED, dan buzzer tetap berfungsi.
- ESP32 mencoba menyambung ulang di latar belakang.

## Tampilan lokal

### LED

- Hijau menyala: slot kosong.
- Merah menyala: slot terisi.

### Buzzer

- 1 kali pendek: plang bergerak.
- 3 kali: mobil datang tapi slot penuh.

### LCD 16x2

- Baris 1: `Slot: KOSONG` atau `Slot: PENUH`.
- Baris 2 sesuai keadaan: `Silakan Masuk`, `Parkir Penuh`, `Hati-hati`, atau `Mode: MANUAL`.

## IoT

### Broker

- EMQX Serverless (cloud, paket gratis).
- TLS di port 8883 dengan username/password.
- ESP32 memakai `WiFiClientSecure` dengan sertifikat CA dari konsol EMQX.

### Topik MQTT

Prefix: `smartparking/<idkelompok>/`

| Topik | Arah | Payload | Retained |
|---|---|---|---|
| `slot` | ESP32 → Node-RED | `KOSONG` / `TERISI` | ya |
| `mode` | ESP32 → Node-RED | `AUTO` / `MANUAL` | ya |
| `mode/set` | Node-RED → ESP32 | `AUTO` / `MANUAL` | tidak |
| `gate/cmd` | Node-RED → ESP32 | `BUKA` / `TUTUP` | tidak |

ESP32 mengirim balik topik `mode` supaya saklar di dashboard selalu sesuai dengan keadaan asli perangkat, misalnya setelah ESP32 offline lalu kembali ke Auto.

### Node-RED

- Berjalan di laptop salah satu anggota, terhubung ke EMQX Serverless.
- Memakai Dashboard 2.0 (`@flowfuse/node-red-dashboard`).
- Isi dashboard:
  - Status slot.
  - Saklar Auto/Manual.
  - Tombol Buka dan Tutup.

## Tooling

- Arduino IDE 2.
- Library: `PubSubClient`, `ESP32Servo`, `LiquidCrystal_I2C`, `WiFiClientSecure`.

## Hasil akhir

- Kode firmware ESP32 dan flow Node-RED.
- Diagram rangkaian.
- Laporan.
- Video demo.

## Pembagian tugas

| Anggota | Tugas |
|---|---|
| 1 | Rangkaian dan maket |
| 2 | Firmware ESP32 |
| 3 | Broker EMQX dan Node-RED |
| 4 | Laporan, diagram, dan video |
