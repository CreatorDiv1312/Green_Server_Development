// #include <Arduino.h>

// #define SOIL_PIN 27
// #define UV_PIN   35
// #define MQ2_PIN  36
// #define PIR_PIN  39
// #define IR_PIN   12
// #define GUN_PIN  26

// HardwareSerial SensorSerial(2); // use UART2
// // TX = 17, RX = 16 by default, but we’ll set custom pins in begin()

// void setup() {
//   Serial.begin(115200);
//   SensorSerial.begin(9600, SERIAL_8N1, 5, 18); // RX=5, TX=18 (as per wiring)

//   pinMode(PIR_PIN, INPUT);
//   pinMode(IR_PIN, INPUT);
//   pinMode(GUN_PIN, INPUT);

//   Serial.println("ESP32-WROOM sensor node initialized");
// }

// void loop() {
//   // --- Read sensors ---
//   float soil = analogRead(SOIL_PIN) / 40.95;     // → 0–100 %
//   float uv = analogRead(UV_PIN) * (10.0 / 4095); // → 0–10 UV index
//   float co = analogRead(MQ2_PIN) / 1000.0;       // ppm example
//   float air = analogRead(MQ2_PIN) / 800.0;       // air quality scaled
//   int pir = digitalRead(PIR_PIN);
//   int ir = digitalRead(IR_PIN);
//   int gun = digitalRead(GUN_PIN);

//   // --- Format as CSV string ---
//   String packet = String(soil, 2) + "," + String(uv, 2) + "," + 
//                   String(co, 2) + "," + String(air, 2) + "," +
//                   String(pir) + "," + String(ir) + "," + String(gun);

//   // --- Send to ESP32-CAM ---
//   SensorSerial.println(packet);
//   Serial.println("Sent to CAM: " + packet);

//   delay(5000);  // every 5 seconds
// }


#include <Arduino.h>
#include <TinyGPSPlus.h>

// ----------------------- PIN CONFIG -----------------------
#define SOIL_PIN   27
#define UV_PIN     35
#define MQ2_PIN    36
#define MQ135_PIN  39
#define RAIN_PIN   25   // Analog pin for rain sensor
#define VIB_PIN    26   // Digital pin for vibration sensor
#define GUN_PIN    26

// GPS and SIM800L UART configuration
#define GPS_RX  2
#define GPS_TX  4
#define SIM_RX  16
#define SIM_TX  17

// UART for sending data to ESP32-CAM
#define CAM_RX  5
#define CAM_TX  18

// ----------------------- OBJECTS --------------------------
HardwareSerial SensorSerial(2);   // UART2 → ESP32-CAM
HardwareSerial GPSSerial(1);      // UART1 → NEO-6M GPS
HardwareSerial SIMSerial(0);      // UART0 → SIM800L
TinyGPSPlus gps;

// ----------------------- SETUP ----------------------------
void setup() {
  Serial.begin(115200);
  SensorSerial.begin(9600, SERIAL_8N1, CAM_RX, CAM_TX);
  GPSSerial.begin(9600, SERIAL_8N1, GPS_RX, GPS_TX);
  SIMSerial.begin(9600, SERIAL_8N1, SIM_RX, SIM_TX);

  pinMode(VIB_PIN, INPUT);
  pinMode(GUN_PIN, INPUT);

  Serial.println("ESP32-WROOM Multi-Sensor Node Initialized (PIR, IR, DHT removed)");
}

// ----------------------- LOOP -----------------------------
void loop() {
  // ---- Analog sensors ----
  float soil = analogRead(SOIL_PIN) / 40.95;     // → 0–100 %
  float uv = analogRead(UV_PIN) * (10.0 / 4095); // → 0–10 UV index
  float mq2_co = analogRead(MQ2_PIN) / 1000.0;   // → ppm approx
  float mq135_air = analogRead(MQ135_PIN) / 1000.0; // → air quality index approx

  // ---- Rain sensor logic ----
  int rainAnalog = analogRead(RAIN_PIN);
  int rainDigital = (rainAnalog < 750) ? 1 : 0; // 1 = rain detected, 0 = dry

  // ---- Digital sensors ----
  int vibration = digitalRead(VIB_PIN);
  int gun = digitalRead(GUN_PIN);

  // ---- GPS Data ----
  while (GPSSerial.available()) {
    gps.encode(GPSSerial.read());
  }

  String lat = "0.0", lon = "0.0";
  if (gps.location.isValid()) {
    lat = String(gps.location.lat(), 6);
    lon = String(gps.location.lng(), 6);
  }

  // ---- Optional SIM800L Health Check ----
  SIMSerial.println("AT");
  delay(100);
  if (SIMSerial.available()) {
    String response = SIMSerial.readString();
    Serial.println("SIM Response: " + response);
  }

  // ---- Format all as CSV ----
  String packet =
      String(soil, 2) + "," +
      String(uv, 2) + "," +
      String(mq2_co, 2) + "," +
      String(mq135_air, 2) + "," +
      String(rainAnalog) + "," +
      String(rainDigital) + "," +
      String(vibration) + "," +
      String(gun) + "," +
      lat + "," + lon;

  // ---- Send to ESP32-CAM ----
  SensorSerial.println(packet);
  Serial.println("Sent to CAM: " + packet);

  delay(5000);  // every 5 seconds
}

