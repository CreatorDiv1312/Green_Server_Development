#include <WiFi.h>
#include <HTTPClient.h>
#include <DHT.h>
#include "esp_camera.h"

// ---------------------------------------------------------
// --------------------- PIN CONFIG -------------------------
// ---------------------------------------------------------
#define PIR_PIN  15
#define IR_PIN   13
#define DHT_PIN  14
#define DHT_TYPE DHT11

// ESP32-CAM AI THINKER CAMERA PINS
#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27

#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5

#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22

// ---------------------------------------------------------
// --------------------- OBJECTS ---------------------------
// ---------------------------------------------------------
HardwareSerial CamSerial(1); // UART1 → RX=3, TX=1
DHT dht(DHT_PIN, DHT_TYPE);

// ---------------------------------------------------------
// --------------------- WIFI CONFIG ------------------------
// ---------------------------------------------------------
const char* ssid = "WIFI_SSID";
const char* password = "WIFI_PASSWORD";

// Sensor JSON Upload URL
const char* serverURL = "https://green-serve-1.onrender.com/";

// Camera Upload URL (FLASK)
const char* cameraURL = "https://green-serve-1-i6mj.onrender.com/camera";

// ---------------------------------------------------------
// ------------------ CAMERA INITIALIZATION -----------------
// ---------------------------------------------------------
void initCamera() {
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;

  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;

  config.frame_size = FRAMESIZE_QVGA; 
  config.jpeg_quality = 12;
  config.fb_count = 2;

  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed: 0x%x\n", err);
  } else {
    Serial.println("Camera initialized successfully.");
  }
}

// ---------------------------------------------------------
// ----------------------- SETUP ---------------------------
// ---------------------------------------------------------
void setup() {
  Serial.begin(115200);
  CamSerial.begin(9600, SERIAL_8N1, 3, 1); // RX=3, TX=1

  pinMode(PIR_PIN, INPUT);
  pinMode(IR_PIN, INPUT);
  dht.begin();

  // Initialize Camera
  initCamera();

  // Wi-Fi setup
  WiFi.begin(ssid, password);
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWi-Fi connected. IP: " + WiFi.localIP().toString());
  Serial.println("ESP32-CAM ready...");
}

// ---------------------------------------------------------
// ------------------ SEND CAMERA FRAME ---------------------
// ---------------------------------------------------------
void sendCameraFrame() {
  camera_fb_t *fb = esp_camera_fb_get();
  if (!fb) {
    Serial.println("Camera capture failed.");
    return;
  }

  HTTPClient http;
  http.begin(cameraURL);
  http.addHeader("Content-Type", "image/jpeg");

  int response = http.POST(fb->buf, fb->len);

  if (response > 0)
    Serial.printf("Camera frame sent: %d\n", response);
  else
    Serial.printf("Camera upload error: %s\n", http.errorToString(response).c_str());

  http.end();
  esp_camera_fb_return(fb);
}

// ---------------------------------------------------------
// ------------------ SEND SENSOR JSON ----------------------
// ---------------------------------------------------------
void sendToServer(String csv) {

  float soil, uv, co, air;
  int rainAnalog, rainDigital, vibration, gun;
  float lat, lon;

  // Parsing CSV from WROOM
  int count = sscanf(csv.c_str(), "%f,%f,%f,%f,%d,%d,%d,%d,%f,%f",
                     &soil, &uv, &co, &air,
                     &rainAnalog, &rainDigital, &vibration, &gun,
                     &lat, &lon);

  if (count != 10) {
    Serial.println("Invalid CSV format, skipping.");
    return;
  }

  // Local sensors on ESP32-CAM
  int pir = digitalRead(PIR_PIN);
  int ir = digitalRead(IR_PIN);
  float temp = dht.readTemperature();
  float hum = dht.readHumidity();

  if (isnan(temp) || isnan(hum)) {
    Serial.println("DHT error, skipping frame.");
    return;
  }

  // Build JSON Payload
  String json = "{";
  json += "\"soil_moisture\":" + String(soil, 2) + ",";
  json += "\"uv_index\":" + String(uv, 2) + ",";
  json += "\"CO_Quality\":" + String(co, 2) + ",";
  json += "\"Air_Quality\":" + String(air, 2) + ",";
  json += "\"rainAnalog\":" + String(rainAnalog) + ",";
  json += "\"rainDigital\":" + String(rainDigital) + ",";
  json += "\"vibration\":" + String(vibration) + ",";
  json += "\"gunfire\":" + String(gun) + ",";
  json += "\"latitude\":" + String(lat, 6) + ",";
  json += "\"longitude\":" + String(lon, 6) + ",";
  json += "\"pir_motion\":" + String(pir) + ",";
  json += "\"ir_detect\":" + String(ir) + ",";
  json += "\"temperature\":" + String(temp, 2) + ",";
  json += "\"humidity\":" + String(hum, 2);
  json += "}";

  Serial.println("Uploading JSON: " + json);

  HTTPClient http;
  http.begin(serverURL);
  http.addHeader("Content-Type", "application/json");

  int code = http.POST(json);

  if (code > 0)
    Serial.printf("Server response (%d): %s\n", code, http.getString().c_str());
  else
    Serial.printf("HTTP Error: %s\n", http.errorToString(code).c_str());

  http.end();
}

// ---------------------------------------------------------
// -------------------- MAIN LOOP ---------------------------
// ---------------------------------------------------------
void loop() {

  // 1️Sensor packet coming from WROOM
  if (CamSerial.available()) {
    String dataLine = CamSerial.readStringUntil('\n');
    dataLine.trim();

    if (dataLine.length() > 0) {
      Serial.println("Received CSV: " + dataLine);
      sendToServer(dataLine);
    }
  }

  // 2️ Send camera frame every 2 seconds
  static unsigned long lastFrame = 0;
  if (millis() - lastFrame > 2000) {
    sendCameraFrame();
    lastFrame = millis();
  }
}

