#include <WiFi.h>
#include <HTTPClient.h>

HardwareSerial CamSerial(1); // UART1 for sensor data

// Wi-Fi credentials
const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";

// Backend endpoint
const char* serverURL = "https://your-render-backend-url.onrender.com/upload"; 

void setup() {
  Serial.begin(115200);
  CamSerial.begin(9600, SERIAL_8N1, 3, 1); // RX=3, TX=1 as you said

  WiFi.begin(ssid, password);
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWi-Fi connected. IP: " + WiFi.localIP().toString());
}

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

void sendToServer(String csv) {
  // Split CSV
  float soil, uv, co, air;
  int pir, ir, gun;
  int count = sscanf(csv.c_str(), "%f,%f,%f,%f,%d,%d,%d",
                     &soil, &uv, &co, &air, &pir, &ir, &gun);
  if (count != 7) {
    Serial.println("Invalid data format, skipping");
    return;
  }

  // --- Build JSON ---
  String json = "{";
  json += "\"soil_moisture\":" + String(soil, 2) + ",";
  json += "\"uv_index\":" + String(uv, 2) + ",";
  json += "\"CO_Quality\":" + String(co, 2) + ",";
  json += "\"AIR_Quality\":" + String(air, 2) + ",";
  json += "\"pir_motion\":" + String(pir) + ",";
  json += "\"ir_detect\":" + String(ir) + ",";
  json += "\"gunfire\":" + String(gun);
  json += "}";

  Serial.println("Uploading JSON: " + json);

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
    Serial.println("Wi-Fi not connected");
  }
}
