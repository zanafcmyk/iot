#include <WiFi.h>
#include <WebServer.h>
#include <math.h>

// Isi dengan nama dan password Wi-Fi/hotspot
const char* WIFI_SSID = "NAMA_WIFI";
const char* WIFI_PASSWORD = "PASSWORD_WIFI";

// Pin sensor
const int PIN_TDS  = 34;  // AO modul TDS ke GPIO34
const int PIN_TRIG = 5;   // TRIG ultrasonik
const int PIN_ECHO = 18;  // ECHO ultrasonik melalui pembagi tegangan

// Pin relay
const int PIN_POMPA_12V = 26; // IN1: pompa isi 12 V
const int PIN_POMPA_5V  = 27; // IN2: pompa buang 5 V

// true = relay aktif-LOW; false = relay aktif-HIGH
const bool RELAY_ACTIVE_LOW = true;
const int POMPA_ON  = RELAY_ACTIVE_LOW ? LOW : HIGH;
const int POMPA_OFF = RELAY_ACTIVE_LOW ? HIGH : LOW;

// Pengaturan sistem
const float BATAS_TDS_PPM = 330.0;
const float JARAK_TARGET_CM = 10.0;
const unsigned long DURASI_BUANG_MS = 15000UL; // Pompa buang 15 detik

// Koreksi satu titik: 400 ppm alat pembanding / 300 ppm sensor = 1.33
const float FAKTOR_KALIBRASI_TDS = 1.33f;

const int TDS_ADC_MIN_VALID = 10;
const int TDS_ADC_MAX_VALID = 4085;

WebServer server(80);

enum StatusSistem {
  MENUNGGU,
  MEMBUANG,
  MENUNGGU_ULTRASONIK,
  MENGISI
};

StatusSistem statusSistem = MENUNGGU;

float nilaiTDS = 0.0;
float teganganTDS = 0.0;
float jarakCM = -1.0;
int nilaiADC_TDS = 0;

bool tdsValid = false;
bool ultrasonikValid = false;
bool serverStarted = false;
unsigned long waktuLogWiFi = 0;

unsigned long waktuMulaiBuang = 0;

void matikanSemuaPompa() {
  digitalWrite(PIN_POMPA_5V, POMPA_OFF);
  digitalWrite(PIN_POMPA_12V, POMPA_OFF);
}

const char* namaStatus() {
  switch (statusSistem) {
    case MENUNGGU:
      return "menunggu";
    case MEMBUANG:
      return "membuang";
    case MENUNGGU_ULTRASONIK:
      return "menunggu_ultrasonik";
    case MENGISI:
      return "mengisi";
    default:
      return "tidak_diketahui";
  }
}

bool bacaTDS(float &hasilPPM) {
  const int JUMLAH_BACAAN = 10;
  uint32_t totalADC = 0;

  for (int i = 0; i < JUMLAH_BACAAN; i++) {
    totalADC += analogRead(PIN_TDS);
    delay(3);
  }

  nilaiADC_TDS = totalADC / JUMLAH_BACAAN;
  teganganTDS = nilaiADC_TDS * (3.3f / 4095.0f);

  if (nilaiADC_TDS <= TDS_ADC_MIN_VALID ||
      nilaiADC_TDS >= TDS_ADC_MAX_VALID) {
    return false;
  }

  // Perkiraan rumus sensor TDS analog pada sekitar 25°C
  hasilPPM =
      (133.42f * teganganTDS * teganganTDS * teganganTDS
       - 255.86f * teganganTDS * teganganTDS
       + 857.39f * teganganTDS) * 0.5f;

  // Faktor kalibrasi satu titik
  hasilPPM *= FAKTOR_KALIBRASI_TDS;

  return isfinite(hasilPPM) && hasilPPM >= 0.0f;
}

bool bacaUltrasonik(float &hasilCM) {
  digitalWrite(PIN_TRIG, LOW);
  delayMicroseconds(3);

  digitalWrite(PIN_TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(PIN_TRIG, LOW);

  unsigned long durasi = pulseIn(PIN_ECHO, HIGH, 30000UL);

  if (durasi == 0) {
    return false;
  }

  hasilCM = durasi * 0.0343f / 2.0f;

  return isfinite(hasilCM) &&
         hasilCM >= 2.0f &&
         hasilCM <= 400.0f;
}

void kirimStatusWeb() {
  char json[256];
  char tdsText[20] = "null";
  char jarakText[20] = "null";

  if (tdsValid) {
    dtostrf(nilaiTDS, 0, 1, tdsText);
  }

  if (ultrasonikValid) {
    dtostrf(jarakCM, 0, 1, jarakText);
  }

  snprintf(
    json,
    sizeof(json),
    "{\"tds\":%s,\"jarak_cm\":%s,\"tds_valid\":%s,"
    "\"ultrasonik_valid\":%s,\"adc_tds\":%d,"
    "\"tegangan_tds\":%.3f,\"status\":\"%s\"}",
    tdsText,
    jarakText,
    tdsValid ? "true" : "false",
    ultrasonikValid ? "true" : "false",
    nilaiADC_TDS,
    teganganTDS,
    namaStatus()
  );

  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.sendHeader("Cache-Control", "no-store");
  server.send(200, "application/json", json);
}

const char* namaStatusWiFi(wl_status_t status) {
  switch (status) {
    case WL_CONNECTED:       return "TERHUBUNG";
    case WL_NO_SSID_AVAIL:   return "SSID tidak ditemukan";
    case WL_CONNECT_FAILED:  return "gagal; cek password/keamanan hotspot";
    case WL_CONNECTION_LOST: return "koneksi terputus";
    case WL_DISCONNECTED:    return "belum terhubung";
    default:                 return "sedang mencoba";
  }
}
void mulaiWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  pinMode(PIN_POMPA_5V, OUTPUT);
  pinMode(PIN_POMPA_12V, OUTPUT);
  matikanSemuaPompa();

  pinMode(PIN_TRIG, OUTPUT);
  pinMode(PIN_ECHO, INPUT);
  digitalWrite(PIN_TRIG, LOW);

  analogReadResolution(12);
  analogSetPinAttenuation(PIN_TDS, ADC_11db);

  server.on("/api/status", HTTP_GET, kirimStatusWeb);
  mulaiWiFi();
  waktuLogWiFi = millis();
  Serial.println("Wi-Fi: mulai menyambung...");

  Serial.println("WATERLY mulai; kedua pompa OFF.");
  Serial.println("Pilih 115200 baud di Serial Monitor.");
  Serial.print("Batas TDS pembuangan: ");
  Serial.print(BATAS_TDS_PPM, 0);
  Serial.println(" ppm");
  Serial.print("Relay aktif-LOW: ");
  Serial.println(RELAY_ACTIVE_LOW ? "ya" : "tidak");
}

void loop() {
  if (WiFi.status() != WL_CONNECTED && serverStarted) {
    serverStarted = false;
    Serial.println("Wi-Fi terputus; ESP32 mencoba menyambung kembali.");
  }

  if (WiFi.status() != WL_CONNECTED && millis() - waktuLogWiFi >= 1000UL) {
    Serial.print("Wi-Fi: ");
    Serial.println(namaStatusWiFi(WiFi.status()));
    waktuLogWiFi = millis();
  }

  if (WiFi.status() == WL_CONNECTED && !serverStarted) {
    server.begin();
    serverStarted = true;

    Serial.print("IP ESP32: http://");
    Serial.println(WiFi.localIP());

    Serial.print("API: http://");
    Serial.print(WiFi.localIP());
    Serial.println("/api/status");
  }

  if (serverStarted) {
    server.handleClient();
  }

  tdsValid = bacaTDS(nilaiTDS);
  ultrasonikValid = bacaUltrasonik(jarakCM);

  switch (statusSistem) {
    case MENUNGGU:
      matikanSemuaPompa();

      if (tdsValid && nilaiTDS >= BATAS_TDS_PPM) {
        digitalWrite(PIN_POMPA_12V, POMPA_OFF);
        digitalWrite(PIN_POMPA_5V, POMPA_ON);

        waktuMulaiBuang = millis();
        statusSistem = MEMBUANG;

        Serial.println("TDS mencapai batas: pompa buang 5 V ON.");
      }
      break;

    case MEMBUANG:
      digitalWrite(PIN_POMPA_12V, POMPA_OFF);
      digitalWrite(PIN_POMPA_5V, POMPA_ON);

      if (millis() - waktuMulaiBuang >= DURASI_BUANG_MS) {
        digitalWrite(PIN_POMPA_5V, POMPA_OFF);
        digitalWrite(PIN_POMPA_12V, POMPA_OFF);

        statusSistem = MENUNGGU_ULTRASONIK;
        Serial.println("Pembuangan selesai; menunggu ultrasonik valid.");
      }
      break;

    case MENUNGGU_ULTRASONIK:
      matikanSemuaPompa();

      if (ultrasonikValid) {
        digitalWrite(PIN_POMPA_12V, POMPA_ON);
        statusSistem = MENGISI;
        Serial.println("Ultrasonik valid: pompa isi 12 V ON.");
      }
      break;

    case MENGISI:
      digitalWrite(PIN_POMPA_5V, POMPA_OFF);

      if (!ultrasonikValid) {
        digitalWrite(PIN_POMPA_12V, POMPA_OFF);
        statusSistem = MENUNGGU_ULTRASONIK;
        Serial.println("Ultrasonik gagal: pompa isi 12 V OFF.");
      } else if (jarakCM <= JARAK_TARGET_CM) {
        digitalWrite(PIN_POMPA_12V, POMPA_OFF);
        statusSistem = MENUNGGU;
        Serial.println("Jarak mencapai target: pompa isi 12 V OFF.");
      } else {
        digitalWrite(PIN_POMPA_12V, POMPA_ON);
      }
      break;
  }

  Serial.print("TDS ADC=");
  Serial.print(nilaiADC_TDS);
  Serial.print(" | Tegangan=");
  Serial.print(teganganTDS, 3);
  Serial.print(" V | TDS=");

  if (tdsValid) {
    Serial.print(nilaiTDS, 1);
    Serial.print(" ppm");
  } else {
    Serial.print("GAGAL");
  }

  Serial.print(" | Jarak=");

  if (ultrasonikValid) {
    Serial.print(jarakCM, 1);
    Serial.print(" cm");
  } else {
    Serial.print("GAGAL");
  }

  Serial.print(" | Status=");
  Serial.println(namaStatus());

  delay(500);
}
