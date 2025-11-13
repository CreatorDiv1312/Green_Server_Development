#include <Arduino.h>

#define SOIL_PIN 27
#define UV_PIN   35
#define MQ2_PIN  36
#define PIR_PIN  39
#define IR_PIN   12
#define GUN_PIN  26

HardwareSerial SensorSerial(2); // use UART2
// TX = 17, RX = 16 by default, but we’ll set custom pins in begin()

void setup() {
  Serial.begin(115200);
  SensorSerial.begin(9600, SERIAL_8N1, 5, 18); // RX=5, TX=18 (as per wiring)

  pinMode(PIR_PIN, INPUT);
  pinMode(IR_PIN, INPUT);
  pinMode(GUN_PIN, INPUT);

  Serial.println("ESP32-WROOM sensor node initialized");
}

void loop() {
  // --- Read sensors ---
  float soil = analogRead(SOIL_PIN) / 40.95;     // → 0–100 %
  float uv = analogRead(UV_PIN) * (10.0 / 4095); // → 0–10 UV index
  float co = analogRead(MQ2_PIN) / 1000.0;       // ppm example
  float air = analogRead(MQ2_PIN) / 800.0;       // air quality scaled
  int pir = digitalRead(PIR_PIN);
  int ir = digitalRead(IR_PIN);
  int gun = digitalRead(GUN_PIN);

  // --- Format as CSV string ---
  String packet = String(soil, 2) + "," + String(uv, 2) + "," + 
                  String(co, 2) + "," + String(air, 2) + "," +
                  String(pir) + "," + String(ir) + "," + String(gun);

  // --- Send to ESP32-CAM ---
  SensorSerial.println(packet);
  Serial.println("Sent to CAM: " + packet);

  delay(5000);  // every 5 seconds
}

