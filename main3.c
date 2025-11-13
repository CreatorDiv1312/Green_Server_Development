#include <WiFi.h>
#include <HTTPClient.h>
#include <DHT.h>
#include <Adafruit_BMP085.h>
#include <TinyGPSPlus.h>
#include <HardwareSerial.h>

// ------------------- PIN CONFIGURATION -------------------
#define DHTPIN 4
#define DHTTYPE DHT11
#define SOIL_PIN 34
#define UV_PIN 35
#define PIR_PIN 13
#define IR_PIN 12
#define GUN_PIN 14

// ------------------- SENSOR OBJECTS -------------------
DHT dht(DHTPIN, DHTTYPE);
Adafruit_BMP085 bmp;
TinyGPSPlus gps;
HardwareSerial GPS_Serial(1); // Using UART1 for GPS

// ------------------- WIFI CONFIG -------------------
const char* ssid = "YOUR_WIFI_SSID";          // 🔹 Replace with  Wi-Fi SSID
const char* password = "YOUR_WIFI_PASSWORD";  // 🔹 Replace with  Wi-Fi password

//  backend URL 
const char* serverURL = "https://green-serve.onrender.com/upload";

// -------------------------------------------------------

void setup() {
  Serial.begin(115200);
  GPS_Serial.begin(9600, SERIAL_8N1, 16, 17); // RX=16, TX=17 for NEO-6M

  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\n✅ WiFi connected!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  dht.begin();
  bmp.begin();

  pinMode(PIR_PIN, INPUT);
  pinMode(IR_PIN, INPUT);
  pinMode(GUN_PIN, INPUT);

  Serial.println("System initialized. Gathering sensor data...");
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("⚠️ WiFi disconnected. Reconnecting...");
    WiFi.reconnect();
    delay(5000);
    return;
  }

  // ------------------- SENSOR READINGS -------------------
  float temperature = dht.readTemperature();
  float humidity = dht.readHumidity();
  float pressure = bmp.readPressure() / 100.0;  // hPa
  float altitude = bmp.readAltitude();
  int soil = analogRead(SOIL_PIN);
  int uv = analogRead(UV_PIN);
  int pir = digitalRead(PIR_PIN);
  int ir = digitalRead(IR_PIN);
  int gun = digitalRead(GUN_PIN);

  // GPS Reading
  while (GPS_Serial.available() > 0) gps.encode(GPS_Serial.read());
  float lat = gps.location.isValid() ? gps.location.lat() : 0.0;
  float lon = gps.location.isValid() ? gps.location.lng() : 0.0;

  // ------------------- BUILD JSON -------------------
  String postData = "{";
  postData += "\"temperature\":" + String(temperature, 2) + ",";
  postData += "\"humidity\":" + String(humidity, 2) + ",";
  postData += "\"pressure\":" + String(pressure, 2) + ",";
  postData += "\"altitude\":" + String(altitude, 2) + ",";
  postData += "\"soil_moisture\":" + String(soil) + ",";
  postData += "\"uv_index\":" + String(uv) + ",";
  postData += "\"pir_motion\":" + String(pir) + ",";
  postData += "\"ir_detect\":" + String(ir) + ",";
  postData += "\"gunfire\":" + String(gun) + ",";
  postData += "\"gps_lat\":" + String(lat, 6) + ",";
  postData += "\"gps_lon\":" + String(lon, 6);
  postData += "}";

  Serial.println("\n📦 Sending JSON Data:");
  Serial.println(postData);

  // ------------------- SEND TO SERVER -------------------
  if (sendDataToServer(postData)) {
    Serial.println("✅ Data sent successfully!");
  } else {
    Serial.println("❌ Failed to send data!");
  }

  delay(15000);  // Send data every 15 seconds
}

// ------------------- FUNCTION DEFINITIONS -------------------
bool sendDataToServer(String data) {
  HTTPClient http;
  http.begin(serverURL);
  http.addHeader("Content-Type", "application/json");

  int httpResponseCode = http.POST(data);

  if (httpResponseCode > 0) {
    Serial.print("Server Response Code: ");
    Serial.println(httpResponseCode);
    String response = http.getString();
    Serial.println("Response: " + response);
    http.end();
    return true;
  } else {
    Serial.print("Error in sending POST: ");
    Serial.println(httpResponseCode);
    http.end();
    return false;
  }
}
