#include <WiFi.h>       // Koneksi Wi-Fi ESP32
#include <WebServer.h>  // Web server untuk mengirim data ke dashboard
#include <time.h>

// Ganti dengan SSID dan password Wi-Fi/hotspot milikmu.
const char* WIFI_SSID = "NAMA_WIFI";
const char* WIFI_PASSWORD = "PASSWORD_WIFI";

// Web server berjalan pada port HTTP 80.
WebServer server(80);

// Pin sensor TDS dan ultrasonik.
const int PIN_TDS = 35;   // Keluaran analog modul TDS (AO ke GPIO35)
const int PIN_TRIG = 5;   // Pin TRIG HC-SR04
const int PIN_ECHO = 18;  // Pin ECHO HC-SR04 melalui pembagi tegangan

// Pin GPIO yang mengendalikan driver pompa.
const int PIN_POMPA_BUANG = 27; // IN2: pompa 5 V untuk membuang air
const int PIN_POMPA_ISI = 26;   // IN1: pompa 12 V untuk mengisi air bersih

// Sesuaikan jika modul relay/MOSFET milikmu aktif LOW.
const int POMPA_ON = HIGH;
const int POMPA_OFF = LOW;

// Nilai TDS ini menjadi batas untuk memulai siklus buang dan isi.
const float BATAS_TDS_PPM = 400.0;

// Pompa isi berhenti saat jarak sensor ke permukaan air 10 cm atau kurang.
const float JARAK_TARGET_CM = 10.0;

// Pembuangan menggunakan timer karena belum ada sensor untuk mendeteksi galon kosong.
const unsigned long DURASI_BUANG_MS = 15000UL;

// Jeda singkat di antara pompa buang dan pompa isi.
const unsigned long JEDA_ANTARPOMPA_MS = 500UL;

// Nilai ADC mendekati 0 atau skala penuh dianggap kabel/sensor tidak valid.
// Atur sesuai hasil kalibrasi. Nilai ADC ESP32 berkisar 0 sampai 4095.
const int TDS_ADC_MIN_VALID = 10;
const int TDS_ADC_MAX_VALID = 4085;

// Status kerja otomatis sistem.
enum StatusSistem {
  MENUNGGU,       // Menunggu TDS mencapai ambang
  MEMBUANG,       // Pompa buang sedang menyala
  JEDA,           // Jeda sebelum pengisian
  MENGISI,        // Pompa isi sedang menyala
  ERROR_SENSOR    // Sensor gagal; semua pompa harus mati
};

// Status awal sistem setelah boot.
StatusSistem statusSistem = MENUNGGU;

// Timer untuk tahap buang dan jeda.
unsigned long waktuMulaiTahap = 0;

// Variabel pembacaan sensor terakhir untuk loop dan endpoint web.
float nilaiTDS = 0.0;
float jarakCM = -1.0;
int nilaiADC_TDS = 0;
bool tdsValid = false;
bool ultrasonikValid = false;

// Membaca ADC beberapa kali dan menghitung perkiraan nilai TDS.
bool bacaTDS(float &hasilPPM) {
  const int JUMLAH_BACAAN = 10;
  uint32_t totalADC = 0;

  // Mengambil beberapa sampel agar pembacaan lebih stabil.
  for (int i = 0; i < JUMLAH_BACAAN; i++) {
    totalADC += analogRead(PIN_TDS);
    delay(5);
  }

  // Menghitung nilai ADC rata-rata.
  nilaiADC_TDS = totalADC / JUMLAH_BACAAN;

  // Menolak nilai yang berada di dekat batas ADC sebagai indikasi sinyal tidak valid.
  if (nilaiADC_TDS <= TDS_ADC_MIN_VALID || nilaiADC_TDS >= TDS_ADC_MAX_VALID) {
    return false;
  }

  // Mengubah nilai ADC menjadi tegangan perkiraan.
  float tegangan = nilaiADC_TDS * (3.3 / 4095.0);

  // Rumus perkiraan modul TDS analog umum pada suhu sekitar 25°C.
  // Sensor perlu dikalibrasi untuk mendapatkan ppm yang akurat.
  hasilPPM = (133.42 * tegangan * tegangan * tegangan
            - 255.86 * tegangan * tegangan
            + 857.39 * tegangan) * 0.5;

  // Menolak hasil tidak masuk akal atau NaN.
  if (!isfinite(hasilPPM) || hasilPPM < 0.0) {
    return false;
  }

  return true;
}

// Mengukur jarak sensor ultrasonik ke permukaan air.
bool bacaUltrasonik(float &hasilJarakCM) {
  // Menyiapkan pin TRIG sebelum mengirim pulsa.
  digitalWrite(PIN_TRIG, LOW);
  delayMicroseconds(3);
  digitalWrite(PIN_TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(PIN_TRIG, LOW);

  // pulseIn mengembalikan 0 jika pantulan tidak diterima dalam 30 ms.
  unsigned long durasi = pulseIn(PIN_ECHO, HIGH, 30000UL);
  if (durasi == 0) {
    return false;
  }

  // Menghitung jarak satu arah berdasarkan waktu tempuh suara.
  hasilJarakCM = durasi * 0.0343 / 2.0;

  // HC-SR04 umumnya hanya valid pada kisaran sekitar 2 sampai 400 cm.
  if (!isfinite(hasilJarakCM) || hasilJarakCM < 2.0 || hasilJarakCM > 400.0) {
    return false;
  }

  return true;
}

// Mematikan kedua pompa sekaligus.
void matikanSemuaPompa() {
  digitalWrite(PIN_POMPA_BUANG, POMPA_OFF);
  digitalWrite(PIN_POMPA_ISI, POMPA_OFF);
}

// Mengembalikan nama status untuk Serial Monitor dan dashboard.
String namaStatus() {
  if (statusSistem == ERROR_SENSOR) {
    if (!tdsValid && !ultrasonikValid) return "sensor_tds_dan_ultrasonik_error";
    if (!tdsValid) return "sensor_tds_error";
    return "sensor_ultrasonik_error";
  }

  switch (statusSistem) {
    case MENUNGGU: return "menunggu";
    case MEMBUANG: return "membuang";
    case JEDA:     return "jeda";
    case MENGISI:  return "mengisi";
    default:       return "tidak_diketahui";
  }
}

// Menangani permintaan dashboard ke alamat /api/status.
void kirimStatusWeb() {
  // Mengirim nilai null ketika sensor terkait dinyatakan gagal.
  String json = "{";
  json += "\"tds\":";
  json += tdsValid ? String(nilaiTDS, 1) : "null";
  json += ",\"jarak_cm\":";
  json += ultrasonikValid ? String(jarakCM, 1) : "null";
  json += ",\"tds_valid\":";
  json += tdsValid ? "true" : "false";
  json += ",\"ultrasonik_valid\":";
  json += ultrasonikValid ? "true" : "false";
  json += ",\"status\":\"" + namaStatus() + "\"";
  json += ",\"updatedAt\":" + String((unsigned long)time(nullptr));
  json += "}";

  // Mengizinkan dashboard lokal membaca API ESP32.
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "application/json", json);
}

// Menyiapkan pin, pompa, Wi-Fi, dan endpoint web.
void setup() {
  Serial.begin(115200);

  // Mengatur pin sensor.
  pinMode(PIN_TRIG, OUTPUT);
  pinMode(PIN_ECHO, INPUT_PULLDOWN); // Menahan ECHO LOW jika kabel terlepas

  // Mengatur pin kendali pompa dan memastikan keduanya mati saat boot.
  pinMode(PIN_POMPA_BUANG, OUTPUT);
  pinMode(PIN_POMPA_ISI, OUTPUT);
  matikanSemuaPompa();

  // Mengatur resolusi ADC 12-bit pada ESP32.
  analogReadResolution(12);

  // Memulai koneksi ke Wi-Fi/hotspot.
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Menghubungkan ke Wi-Fi");

  // Menunggu sampai ESP32 tersambung ke jaringan.
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  // Menampilkan alamat IP ESP32.
  Serial.println();
  Serial.println("Wi-Fi tersambung");
  // Timestamp UTC agar pengirim data dapat meneruskan waktu pembacaan ke Firebase.
  configTime(0, 0, "pool.ntp.org", "time.nist.gov");
  Serial.print("Alamat ESP32: http://");
  Serial.println(WiFi.localIP());
  Serial.print("API status: http://");
  Serial.print(WiFi.localIP());
  Serial.println("/api/status");

  // Menautkan endpoint API ke fungsi penjawab JSON.
  server.on("/api/status", HTTP_GET, kirimStatusWeb);
  server.begin();
  Serial.println("Sistem siap");
}

// Loop utama membaca sensor dan menjalankan urutan pompa.
void loop() {
  // Melayani permintaan dari dashboard.
  server.handleClient();

  // Membaca sensor TDS dan ultrasonik.
  tdsValid = bacaTDS(nilaiTDS);
  ultrasonikValid = bacaUltrasonik(jarakCM);

  // Menampilkan nilai pembacaan atau status gagal pada Serial Monitor.
  Serial.print("TDS: ");
  if (tdsValid) {
    Serial.print(nilaiTDS, 1);
    Serial.print(" ppm");
  } else {
    Serial.print("GAGAL BACA (ADC=");
    Serial.print(nilaiADC_TDS);
    Serial.print(")");
  }

  Serial.print(" | Jarak: ");
  if (ultrasonikValid) {
    Serial.print(jarakCM, 1);
    Serial.print(" cm");
  } else {
    Serial.print("GAGAL BACA");
  }

  Serial.print(" | Status: ");
  Serial.println(namaStatus());

  // Jika salah satu sensor gagal, segera matikan pompa dan hentikan siklus.
  // Pompa tidak menyala lagi sampai kedua sensor memberikan pembacaan valid.
  if (!tdsValid || !ultrasonikValid) {
    matikanSemuaPompa();
    statusSistem = ERROR_SENSOR;
    delay(500);
    return;
  }

  // Setelah kedua sensor kembali normal, sistem kembali ke status menunggu.
  if (statusSistem == ERROR_SENSOR) {
    matikanSemuaPompa();
    statusSistem = MENUNGGU;
    Serial.println("Sensor kembali terbaca; sistem kembali menunggu");
  }

  // Menjalankan tahap kerja berdasarkan status sistem.
  switch (statusSistem) {
    case MENUNGGU:
      // Pastikan semua pompa mati selama menunggu.
      matikanSemuaPompa();

      // Hanya TDS valid yang dapat memulai siklus.
      if (tdsValid && nilaiTDS >= BATAS_TDS_PPM) {
        digitalWrite(PIN_POMPA_BUANG, POMPA_ON);
        waktuMulaiTahap = millis();
        statusSistem = MEMBUANG;
        Serial.println("TDS mencapai ambang; pompa buang menyala");
      }
      break;

    case MEMBUANG:
      // Pompa isi tetap mati selama pembuangan.
      digitalWrite(PIN_POMPA_ISI, POMPA_OFF);

      // Mematikan pompa buang setelah durasi yang ditentukan.
      if (millis() - waktuMulaiTahap >= DURASI_BUANG_MS) {
        digitalWrite(PIN_POMPA_BUANG, POMPA_OFF);
        waktuMulaiTahap = millis();
        statusSistem = JEDA;
        Serial.println("Pembuangan selesai; pompa buang mati");
      }
      break;

    case JEDA:
      // Kedua pompa mati selama jeda antartahap.
      matikanSemuaPompa();

      // Setelah jeda selesai, mulai pengisian.
      if (millis() - waktuMulaiTahap >= JEDA_ANTARPOMPA_MS) {
        digitalWrite(PIN_POMPA_ISI, POMPA_ON);
        statusSistem = MENGISI;
        Serial.println("Pompa isi menyala");
      }
      break;

    case MENGISI:
      // Pompa buang tetap mati selama pengisian.
      digitalWrite(PIN_POMPA_BUANG, POMPA_OFF);

      // Berhenti mengisi saat permukaan air berjarak 10 cm atau kurang.
      if (jarakCM <= JARAK_TARGET_CM) {
        digitalWrite(PIN_POMPA_ISI, POMPA_OFF);
        statusSistem = MENUNGGU;
        Serial.println("Target jarak 10 cm tercapai; pompa isi mati");
      }
      break;

    case ERROR_SENSOR:
      // Status error selalu memastikan pompa mati.
      matikanSemuaPompa();
      break;
  }

  // Jeda pembacaan loop.
  delay(500);
}

