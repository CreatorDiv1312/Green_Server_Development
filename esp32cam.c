#include <WiFi.h>
#include <HTTPClient.h>
#include <DHT.h>

// ----------------------- PIN CONFIG -----------------------
#define PIR_PIN  13
#define IR_PIN   12
#define DHT_PIN  15
#define DHT_TYPE DHT11

// ----------------------- OBJECTS --------------------------
HardwareSerial CamSerial(1); // UART1 → RX=3, TX=1
DHT dht(DHT_PIN, DHT_TYPE);

// ----------------------- WIFI CONFIG ----------------------
const char* ssid = "WIFI_SSID";
const char* password = "WIFI_PASSWORD";
const char* serverURL = "https://green-serve-1.onrender.com/upload";

// ----------------------- SETUP ----------------------------
void setup() {
  Serial.begin(115200);
  CamSerial.begin(9600, SERIAL_8N1, 3, 1);

  pinMode(PIR_PIN, INPUT);
  pinMode(IR_PIN, INPUT);
  dht.begin();

  // Wi-Fi setup
  WiFi.begin(ssid, password);
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWi-Fi connected. IP: " + WiFi.localIP().toString());
  Serial.println("ESP32-CAM ready to receive sensor data...");
}

// ----------------------- MAIN LOOP ------------------------
void loop() {
  if (CamSerial.available()) {
    String dataLine = CamSerial.readStringUntil('\n');
    dataLine.trim();

    if (dataLine.length() > 0) {
      Serial.println("Received: " + dataLine);
      sendToServer(dataLine);
    }
  }
}

// ----------------------- SEND TO SERVER -------------------
void sendToServer(String csv) {
  // Expected CSV from WROOM:
  // soil,uv,co,mq135,rainAnalog,rainDigital,vibration,gun,lat,lon

  float soil, uv, co, air;
  int rainAnalog, rainDigital, vibration, gun;
  float lat, lon;

  int count = sscanf(csv.c_str(), "%f,%f,%f,%f,%d,%d,%d,%d,%f,%f",
                     &soil, &uv, &co, &air,
                     &rainAnalog, &rainDigital, &vibration, &gun,
                     &lat, &lon);

  if (count != 10) {
    Serial.println("Invalid CSV format, skipping");
    return;
  }

  // Local sensors on ESP32-CAM
  int pir = digitalRead(PIR_PIN);
  int ir = digitalRead(IR_PIN);
  float temp = dht.readTemperature();
  float hum = dht.readHumidity();

  if (isnan(temp) || isnan(hum)) {
    Serial.println("DHT read error, skipping this frame");
    return;
  }

  // --- Build full JSON ---
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

  // --- Send HTTP POST ---
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin(serverURL);
    http.addHeader("Content-Type", "application/json");

    int code = http.POST(json);
    if (code > 0) {
      Serial.printf("Server response (%d): %s\n", code, http.getString().c_str());
    } else {
      Serial.printf("HTTP Error: %s\n", http.errorToString(code).c_str());
    }
    http.end();
  } else {
    Serial.println("Wi-Fi disconnected, unable to upload");
  }
}


