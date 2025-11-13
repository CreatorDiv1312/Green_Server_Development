#include <WiFi.h>
#include <HTTPClient.h>
#include <DHT.h>
#include <Adafruit_BMP085.h>
#include <TinyGPSPlus.h>
#include <HardwareSerial.h>

#define DHTPIN 4
#define DHTTYPE DHT11
#define SOIL_PIN 34
#define UV_PIN 35
#define PIR_PIN 13
#define IR_PIN 12
#define GUN_PIN 14

DHT dht(DHTPIN, DHTTYPE);
Adafruit_BMP085 bmp;
TinyGPSPlus gps;
HardwareSerial GPS_Serial(1);
HardwareSerial SIM800L(2);

// Replace with API endpoint is left
const char* serverURL = "https://green-serve-1.onrender.com/upload";

void setup() {
  Serial.begin(115200);
  SIM800L.begin(9600, SERIAL_8N1, 27, 26); // TX,RX for SIM800L, cross check it, it might be wrong
  GPS_Serial.begin(9600, SERIAL_8N1, 16, 17); // RX,TX for GPS, cross check it, it might be wrong
  
  dht.begin();
  bmp.begin();

  pinMode(PIR_PIN, INPUT);
  pinMode(IR_PIN, INPUT);
  pinMode(GUN_PIN, INPUT);

  Serial.println("System initialized. Gathering sensor data...");
  delay(2000);
}

void loop() {
  float temperature = dht.readTemperature();
  float humidity = dht.readHumidity();
  float pressure = bmp.readPressure() / 100.0;  // hPa
  float altitude = bmp.readAltitude();
  int soil = analogRead(SOIL_PIN);
  int uv = analogRead(UV_PIN); //This sensor might be missing so the data might not come.
  int pir = digitalRead(PIR_PIN);
  int ir = digitalRead(IR_PIN);
  int gun = digitalRead(GUN_PIN);

  // GPS reading
  while (GPS_Serial.available() > 0) gps.encode(GPS_Serial.read());
  float lat = gps.location.isValid() ? gps.location.lat() : 0.0;
  float lon = gps.location.isValid() ? gps.location.lng() : 0.0;

  // Prepare JSON data
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

  Serial.println("Data JSON: " + postData);

  sendDataToServer(postData);
  delay(15000);  // send every 15 seconds ( for testing, timings may increase as per the need)
}

void sendDataToServer(String data) {
  Serial.println("Connecting to network...");

  // Basic GPRS setup with AT commands
  SIM800L.println("AT");
  delay(1000);
  SIM800L.println("AT+SAPBR=3,1,\"CONTYPE\",\"GPRS\"");
  delay(1000);
  SIM800L.println("AT+SAPBR=3,1,\"APN\",\"airtelgprs.com\""); // change APN ( if needed, i was not having any idea of it so i randomly inserted it)
  delay(2000);
  SIM800L.println("AT+SAPBR=1,1");
  delay(3000);
  SIM800L.println("AT+HTTPINIT");
  delay(1000);
  SIM800L.println("AT+HTTPPARA=\"CID\",1");
  delay(500);

  SIM800L.print("AT+HTTPPARA=\"URL\",\"");
  SIM800L.print(serverURL);
  SIM800L.println("\"");
  delay(1000);

  SIM800L.println("AT+HTTPPARA=\"CONTENT\",\"application/json\"");
  delay(500);
  SIM800L.print("AT+HTTPDATA=");
  SIM800L.print(data.length());
  SIM800L.println(",10000");
  delay(1000);
  SIM800L.print(data);
  delay(2000);
  SIM800L.println("AT+HTTPACTION=1");
  delay(5000);

  Serial.println("Data sent to server!");
  SIM800L.println("AT+HTTPTERM");
  delay(500);
}

