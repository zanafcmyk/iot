#include <WiFi.h>
#include <WebServer.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <Preferences.h>
#include <math.h>
#include <time.h>
#include <string.h>

// Isi dengan nama dan password Wi-Fi/hotspot.
const char* WIFI_SSID = "Cek";
const char* WIFI_PASSWORD = "I II III IV V VI VII VIII IX X";
// Konfigurasi Firebase proyek Waterly.
// API key Firebase web bukan kata sandi; Rules tetap membatasi akses tulis.
const char* FIREBASE_API_KEY = "AIzaSyDZesr4m5HI9NR2GvGbVSJBw_TOx-HxUjA";
const char* FIREBASE_DATABASE_URL = "https://monitoring-nila-f3de9-default-rtdb.asia-southeast1.firebasedatabase.app";

// Sertifikat akar GTS Root R4 untuk rantai HTTPS Firebase/Google saat ini.
static const char FIREBASE_ROOT_CA[] PROGMEM = R"CERT(-----BEGIN CERTIFICATE-----
MIICCTCCAY6gAwIBAgINAgPlwGjvYxqccpBQUjAKBggqhkjOPQQDAzBHMQswCQYD
VQQGEwJVUzEiMCAGA1UEChMZR29vZ2xlIFRydXN0IFNlcnZpY2VzIExMQzEUMBIG
A1UEAxMLR1RTIFJvb3QgUjQwHhcNMTYwNjIyMDAwMDAwWhcNMzYwNjIyMDAwMDAw
WjBHMQswCQYDVQQGEwJVUzEiMCAGA1UEChMZR29vZ2xlIFRydXN0IFNlcnZpY2Vz
IExMQzEUMBIGA1UEAxMLR1RTIFJvb3QgUjQwdjAQBgcqhkjOPQIBBgUrgQQAIgNi
AATzdHOnaItgrkO4NcWBMHtLSZ37wWHO5t5GvWvVYRg1rkDdc/eJkTBa6zzuhXyi
QHY7qca4R9gq55KRanPpsXI5nymfopjTX15YhmUPoYRlBtHci8nHc8iMai/lxKvR
HYqjQjBAMA4GA1UdDwEB/wQEAwIBhjAPBgNVHRMBAf8EBTADAQH/MB0GA1UdDgQW
BBSATNbrdP9JNqPV2Py1PsVq8JQdjDAKBggqhkjOPQQDAwNpADBmAjEA6ED/g94D
9J+uHXqnLrmvT/aDHQ4thQEd0dlq7A/Cr8deVl5c1RxYIigL9zC2L7F8AjEA8GE8
p/SgguMh1YQdc4acLa/KNJvxn7kjNuK8YAOdgLOaVsjh4rsUecrNIdSUtUlD
-----END CERTIFICATE-----
)CERT";

// Sertifikat akar GTS Root R1 untuk host Realtime Database Firebase.
static const char FIREBASE_DATABASE_ROOT_CA[] PROGMEM = R"CERT(-----BEGIN CERTIFICATE-----
MIIFVzCCAz+gAwIBAgINAgPlk28xsBNJiGuiFzANBgkqhkiG9w0BAQwFADBHMQsw
CQYDVQQGEwJVUzEiMCAGA1UEChMZR29vZ2xlIFRydXN0IFNlcnZpY2VzIExMQzEU
MBIGA1UEAxMLR1RTIFJvb3QgUjEwHhcNMTYwNjIyMDAwMDAwWhcNMzYwNjIyMDAw
MDAwWjBHMQswCQYDVQQGEwJVUzEiMCAGA1UEChMZR29vZ2xlIFRydXN0IFNlcnZp
Y2VzIExMQzEUMBIGA1UEAxMLR1RTIFJvb3QgUjEwggIiMA0GCSqGSIb3DQEBAQUA
A4ICDwAwggIKAoICAQC2EQKLHuOhd5s73L+UPreVp0A8of2C+X0yBoJx9vaMf/vo
27xqLpeXo4xL+Sv2sfnOhB2x+cWX3u+58qPpvBKJXqeqUqv4IyfLpLGcY9vXmX7w
Cl7raKb0xlpHDU0QM+NOsROjyBhsS+z8CZDfnWQpJSMHobTSPS5g4M/SCYe7zUjw
TcLCeoiKu7rPWRnWr4+wB7CeMfGCwcDfLqZtbBkOtdh+JhpFAz2weaSUKK0Pfybl
qAj+lug8aJRT7oM6iCsVlgmy4HqMLnXWnOunVmSPlk9orj2XwoSPwLxAwAtcvfaH
szVsrBhQf4TgTM2S0yDpM7xSma8ytSmzJSq0SPly4cpk9+aCEI3oncKKiPo4Zor8
Y/kB+Xj9e1x3+naH+uzfsQ55lVe0vSbv1gHR6xYKu44LtcXFilWr06zqkUspzBmk
MiVOKvFlRNACzqrOSbTqn3yDsEB750Orp2yjj32JgfpMpf/VjsPOS+C12LOORc92
wO1AK/1TD7Cn1TsNsYqiA94xrcx36m97PtbfkSIS5r762DL8EGMUUXLeXdYWk70p
aDPvOmbsB4om3xPXV2V4J95eSRQAogB/mqghtqmxlbCluQ0WEdrHbEg8QOB+DVrN
VjzRlwW5y0vtOUucxD/SVRNuJLDWcfr0wbrM7Rv1/oFB2ACYPTrIrnqYNxgFlQID
AQABo0IwQDAOBgNVHQ8BAf8EBAMCAYYwDwYDVR0TAQH/BAUwAwEB/zAdBgNVHQ4E
FgQU5K8rJnEaK0gnhS9SZizv8IkTcT4wDQYJKoZIhvcNAQEMBQADggIBAJ+qQibb
C5u+/x6Wki4+omVKapi6Ist9wTrYggoGxval3sBOh2Z5ofmmWJyq+bXmYOfg6LEe
QkEzCzc9zolwFcq1JKjPa7XSQCGYzyI0zzvFIoTgxQ6KfF2I5DUkzps+GlQebtuy
h6f88/qBVRRiClmpIgUxPoLW7ttXNLwzldMXG+gnoot7TiYaelpkttGsN/H9oPM4
7HLwEXWdyzRSjeZ2axfG34arJ45JK3VmgRAhpuo+9K4l/3wV3s6MJT/KYnAK9y8J
ZgfIPxz88NtFMN9iiMG1D53Dn0reWVlHxYciNuaCp+0KueIHoI17eko8cdLiA6Ef
MgfdG+RCzgwARWGAtQsgWSl4vflVy2PFPEz0tv/bal8xa5meLMFrUKTX5hgUvYU/
Z6tGn6D/Qqc6f1zLXbBwHSs09dR2CQzreExZBfMzQsNhFRAbd03OIozUhfJFfbdT
6u9AWpQKXCBfTkBdYiJ23//OYb2MI3jSNwLgjt7RETeJ9r/tSQdirpLsQBqvFAnZ
0E6yove+7u7Y/9waLd64NnHi/Hm3lCXRSHNboTXns5lndcEZOitHTtNCjv0xyBZm
2tIMPNuzjsmhDYAPexZ3FL//2wmUspO8IFgV6dtxQ/PeEMMA3KgqlbbC1j+Qa3bb
bP6MvPJwNQzcmRk13NfIRmPVNnGuV/u3gm3c
-----END CERTIFICATE-----
)CERT";

// Pin sensor
const int PIN_TDS = 34;       // AO modul TDS (kabel AO dipasang ke GPIO34)
const int PIN_TRIG = 5;       // TRIG HC-SR04
const int PIN_ECHO = 18;      // ECHO HC-SR04 lewat pembagi tegangan

// Pemetaan sesuai sketch sebelumnya:
// IN1 / pompa isi 12 V -> GPIO26
// IN2 / pompa buang 5 V -> GPIO27
const int PIN_POMPA_12V = 26;
const int PIN_POMPA_5V = 27;

// false = relay aktif-HIGH; true = relay aktif-LOW.
// Uji relay tanpa catu daya pompa untuk memastikan polaritasnya.
const bool RELAY_ACTIVE_LOW = true;
const int POMPA_ON = RELAY_ACTIVE_LOW ? LOW : HIGH;
const int POMPA_OFF = RELAY_ACTIVE_LOW ? HIGH : LOW;

const float BATAS_TDS_PPM = 400.0;
const float JARAK_TARGET_CM = 10.0;
const unsigned long DURASI_BUANG_MS = 15000UL;
const int TDS_ADC_MIN_VALID = 10;

WebServer server(80);
Preferences firebasePreferences;
String firebaseIdToken;
String firebaseRefreshToken;
String firebaseDeviceUid;
unsigned long firebaseTokenExpiresAt = 0;
unsigned long lastFirebaseWrite = 0;
unsigned long lastFirebaseAuthAttempt = 0;
bool firebaseReady = false;

enum StatusSistem { MENUNGGU, MEMBUANG, MENGISI, ERROR_SENSOR };
StatusSistem statusSistem = MENUNGGU;

float nilaiTDS = 0.0;
float teganganTDS = 0.0;
float jarakCM = -1.0;
int nilaiADC_TDS = 0;
bool tdsValid = false;
bool ultrasonikValid = false;
bool serverStarted = false;
unsigned long waktuMulaiBuang = 0;

void matikanSemuaPompa() {
  digitalWrite(PIN_POMPA_5V, POMPA_OFF);
  digitalWrite(PIN_POMPA_12V, POMPA_OFF);
}

const char* namaStatus() {
  switch (statusSistem) {
    case MENUNGGU: return "menunggu";
    case MEMBUANG: return "membuang";
    case MENGISI: return "mengisi";
    case ERROR_SENSOR: return "sensor_error";
    default: return "tidak_diketahui";
  }
}

bool bacaTDS(float &hasilPPM) {
  const int JUMLAH_BACAAN = 20;
  uint32_t totalADC = 0;
  uint32_t totalMilliVolt = 0;

  for (int i = 0; i < JUMLAH_BACAAN; i++) {
    totalADC += analogRead(PIN_TDS);
    totalMilliVolt += analogReadMilliVolts(PIN_TDS);
    delay(5);
  }

  nilaiADC_TDS = totalADC / JUMLAH_BACAAN;
  teganganTDS = (totalMilliVolt / (float)JUMLAH_BACAAN) / 1000.0f;

  // Jangan ubah offset tegangan menjadi ppm jika ADC mentah tidak menangkap sinyal.
  if (nilaiADC_TDS <= TDS_ADC_MIN_VALID || teganganTDS < 0.10f) {
    return false;
  }

  // Perkiraan untuk modul TDS analog umum, belum dikalibrasi.
  hasilPPM = (133.42f * teganganTDS * teganganTDS * teganganTDS
             - 255.86f * teganganTDS * teganganTDS
             + 857.39f * teganganTDS) * 0.5f;

  return isfinite(hasilPPM) && hasilPPM >= 0.0f;
}

bool bacaUltrasonik(float &hasilCM) {
  digitalWrite(PIN_TRIG, LOW);
  delayMicroseconds(3);
  digitalWrite(PIN_TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(PIN_TRIG, LOW);

  unsigned long durasi = pulseIn(PIN_ECHO, HIGH, 30000UL);
  if (durasi == 0) return false;

  hasilCM = durasi * 0.0343f / 2.0f;
  return isfinite(hasilCM) && hasilCM >= 2.0f && hasilCM <= 400.0f;
}

void kirimStatusWeb() {
  char json[320];
  char tdsText[20] = "null";
  char jarakText[20] = "null";
  if (tdsValid) dtostrf(nilaiTDS, 0, 1, tdsText);
  if (ultrasonikValid) dtostrf(jarakCM, 0, 1, jarakText);

  snprintf(json, sizeof(json),
    "{\"tds\":%s,\"jarak_cm\":%s,\"tds_valid\":%s,"
    "\"ultrasonik_valid\":%s,\"adc_tds\":%d,"
    "\"tegangan_tds\":%.3f,\"status\":\"%s\",\"updatedAt\":%lu}",
    tdsText, jarakText,
    tdsValid ? "true" : "false",
    ultrasonikValid ? "true" : "false",
    nilaiADC_TDS, teganganTDS, namaStatus(),
    (unsigned long)time(nullptr));

  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.sendHeader("Cache-Control", "no-store");
  server.send(200, "application/json", json);
}

void mulaiWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
}

// Mengambil field teks sederhana, seperti idToken, dari respons JSON Firebase.
String ambilFieldJson(const String &json, const char* namaField) {
  String kunci = String("\"") + namaField + "\"";
  int posisi = json.indexOf(kunci);
  if (posisi < 0) return "";
  posisi = json.indexOf(':', posisi + kunci.length());
  if (posisi < 0) return "";
  posisi = json.indexOf('"', posisi + 1);
  if (posisi < 0) return "";
  int akhir = json.indexOf('"', posisi + 1);
  if (akhir < 0) return "";
  return json.substring(posisi + 1, akhir);
}

// Mengubah karakter khusus agar aman dikirim dalam form HTTPS.
String encodeFormValue(const String &value) {
  String hasil;
  const char hex[] = "0123456789ABCDEF";
  for (size_t i = 0; i < value.length(); i++) {
    uint8_t c = (uint8_t)value[i];
    if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
        (c >= '0' && c <= '9') || c == '-' || c == '_' || c == '.' || c == '~') {
      hasil += (char)c;
    } else {
      hasil += '%';
      hasil += hex[c >> 4];
      hasil += hex[c & 0x0F];
    }
  }
  return hasil;
}

// Melakukan POST atau PUT HTTPS dan mengembalikan kode serta isi respons.
int firebaseHttpsRequest(const char* method, const String &url,
                         const String &contentType, const String &body,
                         String &responseBody) {
  WiFiClientSecure client;
  // Realtime Database memakai rantai sertifikat berbeda dari Auth Firebase.
  if (url.indexOf("firebasedatabase.app") >= 0) {
    client.setCACert(FIREBASE_DATABASE_ROOT_CA);
  } else {
    client.setCACert(FIREBASE_ROOT_CA);
  }

  HTTPClient https;
  https.setConnectTimeout(15000);
  https.setTimeout(15000);
  if (!https.begin(client, url)) {
    char tlsError[128] = {0};
    client.lastError(tlsError, sizeof(tlsError));
    Serial.print("HTTPS begin gagal: ");
    Serial.println(tlsError);
    return -1;
  }
  https.addHeader("Content-Type", contentType);

  int httpCode;
  if (strcmp(method, "PUT") == 0) {
    httpCode = https.PUT(body);
  } else {
    httpCode = https.POST(body);
  }

  if (httpCode < 0) {
    Serial.print("HTTPS transport: ");
    Serial.println(https.errorToString(httpCode));
    char tlsError[128] = {0};
    client.lastError(tlsError, sizeof(tlsError));
    if (tlsError[0] != '\0') {
      Serial.print("TLS detail: ");
      Serial.println(tlsError);
    }
  }

  responseBody = https.getString();
  https.end();
  return httpCode;
}

// Membuat akun anonim satu kali, lalu menyimpan refresh token di memori ESP32.
bool buatAkunFirebaseAnonim() {
  String responseBody;
  String url = String("https://identitytoolkit.googleapis.com/v1/accounts:signUp?key=") + FIREBASE_API_KEY;
  int httpCode = firebaseHttpsRequest("POST", url, "application/json",
                                      "{\"returnSecureToken\":true}", responseBody);

  if (httpCode != 200) {
    Serial.print("Firebase Auth gagal, HTTP ");
    Serial.println(httpCode);
    return false;
  }

  firebaseIdToken = ambilFieldJson(responseBody, "idToken");
  firebaseRefreshToken = ambilFieldJson(responseBody, "refreshToken");
  firebaseDeviceUid = ambilFieldJson(responseBody, "localId");
  if (firebaseIdToken.isEmpty() || firebaseRefreshToken.isEmpty() || firebaseDeviceUid.isEmpty()) {
    Serial.println("Respons Firebase Auth tidak lengkap.");
    return false;
  }

  firebasePreferences.putString("refresh", firebaseRefreshToken);
  firebasePreferences.putString("uid", firebaseDeviceUid);
  firebaseTokenExpiresAt = millis() + 3600000UL;
  firebaseReady = true;

  Serial.print("Firebase Auth berhasil. UID perangkat: ");
  Serial.println(firebaseDeviceUid);
  Serial.println("UID ini perlu diizinkan pada .write Rules deviceData/latest.");
  return true;
}

// Memperbarui ID token memakai refresh token yang tersimpan.
bool segarkanTokenFirebase() {
  if (firebaseRefreshToken.isEmpty()) return false;

  String responseBody;
  String url = String("https://securetoken.googleapis.com/v1/token?key=") + FIREBASE_API_KEY;
  String body = "grant_type=refresh_token&refresh_token=" + encodeFormValue(firebaseRefreshToken);
  int httpCode = firebaseHttpsRequest("POST", url, "application/x-www-form-urlencoded", body, responseBody);

  if (httpCode != 200) {
    Serial.print("Penyegaran token Firebase gagal, HTTP ");
    Serial.println(httpCode);
    String authError = ambilFieldJson(responseBody, "message");
    if (!authError.isEmpty()) {
      Serial.print("Firebase Auth detail: ");
      Serial.println(authError);
    }

    // Jika refresh token sudah tidak berlaku, buat akun anonim baru saat percobaan berikutnya.
    if (authError == "INVALID_REFRESH_TOKEN" || authError == "USER_NOT_FOUND") {
      firebasePreferences.remove("refresh");
      firebasePreferences.remove("uid");
      firebaseRefreshToken = "";
      firebaseDeviceUid = "";
      Serial.println("Token lama dihapus; akun Firebase baru akan dibuat.");
    }
    return false;
  }

  String tokenBaru = ambilFieldJson(responseBody, "id_token");
  String refreshBaru = ambilFieldJson(responseBody, "refresh_token");
  String uidBaru = ambilFieldJson(responseBody, "user_id");
  if (tokenBaru.isEmpty() || refreshBaru.isEmpty()) return false;

  firebaseIdToken = tokenBaru;
  firebaseRefreshToken = refreshBaru;
  if (!uidBaru.isEmpty()) firebaseDeviceUid = uidBaru;
  firebasePreferences.putString("refresh", firebaseRefreshToken);
  firebasePreferences.putString("uid", firebaseDeviceUid);
  firebaseTokenExpiresAt = millis() + 3600000UL;
  firebaseReady = true;
  Serial.println("Token Firebase diperbarui.");
  Serial.print("UID Firebase aktif dari token: ");
  Serial.println(firebaseDeviceUid);
  return true;
}

// Login ulang dengan akun anonim yang sama atau buat akun perangkat pertama kali.
bool sambungkanFirebase() {
  if (!firebaseRefreshToken.isEmpty()) {
    // Jangan membuat akun baru saat jaringan sedang gangguan; pakai UID yang sama.
    return segarkanTokenFirebase();
  }
  return buatAkunFirebaseAnonim();
}

// Mengirim pembacaan terbaru dan waktu server Firebase ke deviceData/latest.
bool kirimKeFirebase() {
  if (!firebaseReady || firebaseIdToken.isEmpty()) return false;

  // Perbarui token sebelum masa berlakunya habis.
  if ((long)(millis() - (firebaseTokenExpiresAt - 120000UL)) >= 0) {
    if (!segarkanTokenFirebase()) {
      firebaseReady = false;
      return false;
    }
  }

  char tdsText[20] = "null";
  char jarakText[20] = "null";
  if (tdsValid) dtostrf(nilaiTDS, 0, 1, tdsText);
  if (ultrasonikValid) dtostrf(jarakCM, 0, 1, jarakText);

  char json[320];
  snprintf(json, sizeof(json),
    "{\"tds\":%s,\"jarak_cm\":%s,\"tds_valid\":%s,"
    "\"ultrasonik_valid\":%s,\"adc_tds\":%d,\"tegangan_tds\":%.3f,"
    "\"status\":\"%s\",\"updatedAt\":{\".sv\":\"timestamp\"}}",
    tdsText, jarakText,
    tdsValid ? "true" : "false",
    ultrasonikValid ? "true" : "false",
    nilaiADC_TDS, teganganTDS, namaStatus());

  String url = String(FIREBASE_DATABASE_URL) +
    "/deviceData/latest.json?auth=" + firebaseIdToken;
  String responseBody;
  int httpCode = firebaseHttpsRequest("PUT", url, "application/json", String(json), responseBody);

  if (httpCode == 200) {
    Serial.println("Data sensor berhasil dikirim ke Firebase.");
    return true;
  }

  Serial.print("Kirim Firebase gagal, HTTP ");
  Serial.println(httpCode);
  if (responseBody.indexOf("Permission denied") >= 0) {
    Serial.println("UID perangkat belum diizinkan pada Firebase Rules.");
    Serial.print("UID perangkat: ");
    Serial.println(firebaseDeviceUid);
  }
  firebaseReady = false;
  return false;
}

// Tugas terpisah agar koneksi HTTPS Firebase tidak menghentikan kontrol pompa.
void tugasFirebase(void* parameter) {
  firebasePreferences.begin("waterly", false);
  firebaseRefreshToken = firebasePreferences.getString("refresh", "");
  firebaseDeviceUid = firebasePreferences.getString("uid", "");
  if (!firebaseDeviceUid.isEmpty()) {
    Serial.print("UID perangkat tersimpan: ");
    Serial.println(firebaseDeviceUid);
  }
  unsigned long percobaanAuthTerakhir = millis() - 60000UL;

  while (true) {
    if (WiFi.status() != WL_CONNECTED) {
      vTaskDelay(pdMS_TO_TICKS(1000));
      continue;
    }

    // Tunggu NTP agar sertifikat HTTPS bisa diverifikasi memakai waktu yang benar.
    if (time(nullptr) < 1700000000) {
      vTaskDelay(pdMS_TO_TICKS(1000));
      continue;
    }

    if (!firebaseReady && millis() - percobaanAuthTerakhir >= 60000UL) {
      percobaanAuthTerakhir = millis();
      sambungkanFirebase();
    }

    if (firebaseReady && millis() - lastFirebaseWrite >= 5000UL) {
      lastFirebaseWrite = millis();
      kirimKeFirebase();
    }

    vTaskDelay(pdMS_TO_TICKS(500));
  }
}

void setup() {
  Serial.begin(115200);
  delay(500);

  // Set level OFF sebelum menjadikan pin sebagai OUTPUT.
  digitalWrite(PIN_POMPA_5V, POMPA_OFF);
  digitalWrite(PIN_POMPA_12V, POMPA_OFF);
  pinMode(PIN_POMPA_5V, OUTPUT);
  pinMode(PIN_POMPA_12V, OUTPUT);
  matikanSemuaPompa();

  pinMode(PIN_TRIG, OUTPUT);
  pinMode(PIN_ECHO, INPUT);
  digitalWrite(PIN_TRIG, LOW);

  analogReadResolution(12);
  analogSetPinAttenuation(PIN_TDS, ADC_2_5db);

  // Inisialisasi Wi-Fi lebih dulu. Web server dimulai setelah tersambung.
  server.on("/api/status", HTTP_GET, kirimStatusWeb);
  mulaiWiFi();

  // Firebase berjalan pada task lain supaya pembacaan sensor dan relay tetap lancar.
  xTaskCreatePinnedToCore(tugasFirebase, "tugasFirebase", 12000, nullptr, 1, nullptr, 0);

  Serial.println("Sistem dimulai; pompa dalam kondisi OFF.");
  Serial.println("Serial Monitor: 115200 baud.");
  Serial.print("Polaritas relay aktif-LOW: ");
  Serial.println(RELAY_ACTIVE_LOW ? "ya" : "tidak");
}

void loop() {
  if (WiFi.status() == WL_CONNECTED && !serverStarted) {
    // Sinkronkan waktu UTC untuk timestamp dan verifikasi HTTPS Firebase.
    configTime(0, 0, "pool.ntp.org", "time.nist.gov");
    server.begin();
    serverStarted = true;
    Serial.print("Wi-Fi tersambung. IP ESP32: http://");
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

  if (!tdsValid || !ultrasonikValid) {
    matikanSemuaPompa();
    statusSistem = ERROR_SENSOR;
  } else {
    if (statusSistem == ERROR_SENSOR) {
      matikanSemuaPompa();
      statusSistem = MENUNGGU;
    }

    switch (statusSistem) {
      case MENUNGGU:
        matikanSemuaPompa();
        if (nilaiTDS >= BATAS_TDS_PPM) {
          // TDS tinggi: pompa buang 5 V bekerja dulu.
          digitalWrite(PIN_POMPA_5V, POMPA_ON);
          waktuMulaiBuang = millis();
          statusSistem = MEMBUANG;
          Serial.println("TDS >= 400 ppm: pompa 5 V ON.");
        }
        break;

      case MEMBUANG:
        digitalWrite(PIN_POMPA_12V, POMPA_OFF);
        if (millis() - waktuMulaiBuang >= DURASI_BUANG_MS) {
          // Setelah durasi pembuangan, 5 V OFF lalu 12 V ON.
          digitalWrite(PIN_POMPA_5V, POMPA_OFF);
          digitalWrite(PIN_POMPA_12V, POMPA_ON);
          statusSistem = MENGISI;
          Serial.println("Pembuangan selesai: pompa 5 V OFF, pompa 12 V ON.");
        }
        break;

      case MENGISI:
        digitalWrite(PIN_POMPA_5V, POMPA_OFF);
        if (jarakCM <= JARAK_TARGET_CM) {
          digitalWrite(PIN_POMPA_12V, POMPA_OFF);
          statusSistem = MENUNGGU;
          Serial.println("Jarak <= 10 cm: pompa 12 V OFF.");
        }
        break;

      case ERROR_SENSOR:
        matikanSemuaPompa();
        break;
    }
  }

  Serial.print("TDS ADC=");
  Serial.print(nilaiADC_TDS);
  Serial.print(" Tegangan=");
  Serial.print(teganganTDS, 3);
  Serial.print(" V TDS=");
  if (tdsValid) Serial.print(nilaiTDS, 1);
  else Serial.print("GAGAL");
  Serial.print(" ppm Jarak=");
  if (ultrasonikValid) Serial.print(jarakCM, 1);
  else Serial.print("GAGAL");
  Serial.print(" cm Status=");
  Serial.println(namaStatus());

  delay(500);
}
