#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>

// === Pin Configuration ===
#define TRIG_PIN 13
#define ECHO_PIN 12

// === Sensors ===
Adafruit_BME280 bme; // I2C

// === Wi-Fi Credentials ===
const char* wifi_ssid = "POCO X3 NFC";
const char* wifi_password = "12345679";

// === Google Apps Script Web App URL ===
const char* googleScriptURL = "https://script.google.com/macros/s/AKfycby-haceUB3idc8yUnmIYpLYie18hYconyHx0B1rldNszZ6d1rHV7SoiPD6oO7vCEi_pUw/exec";

// === Timing ===
const unsigned long SEND_INTERVAL = 5000; // 5 seconds

// === Get Distance in cm ===
float getDistanceCM() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  long duration = pulseIn(ECHO_PIN, HIGH, 30000);
  if (duration == 0) return -1;
  return duration * 0.034 / 2.0;
}

// === Send Data to Google Sheets (POST JSON) ===
void sendDataToGoogleSheet(float distance, float temperature, float humidity, float pressure) {
  if (WiFi.status() == WL_CONNECTED) {
    WiFiClientSecure client;
    client.setInsecure();

    HTTPClient http;
    http.begin(client, googleScriptURL);
    http.addHeader("Content-Type", "application/json");

    // Create JSON payload
    String jsonData = "{\"distance\":" + String(distance, 2) +
                      ",\"temperature\":" + String(temperature, 2) +
                      ",\"humidity\":" + String(humidity, 2) +
                      ",\"pressure\":" + String(pressure, 2) + "}";

    Serial.println("\nSending data to Google Sheets...");
    Serial.println(jsonData);

    int httpCode = http.POST(jsonData);
    String payload = http.getString();

    Serial.printf("HTTP Response code: %d\n", httpCode);
    Serial.println("Response: " + payload);

    http.end();
  } else {
    Serial.println("WiFi disconnected, skipping data send.");
  }
}

// === Setup ===
void setup() {
  Serial.begin(115200);
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  Serial.println("Initializing BME280...");
  if (!bme.begin(0x76)) {
    Serial.println("BME280 not found! Try changing address to 0x77.");
    while (1);
  }
  Serial.println("BME280 ready.");

  WiFi.begin(wifi_ssid, wifi_password);
  Serial.print("Connecting to WiFi");
  int retries = 0;
  while (WiFi.status() != WL_CONNECTED && retries < 30) {
    delay(500);
    Serial.print(".");
    retries++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi connected!");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\nWiFi connection failed.");
  }
}

// === Main Loop ===
void loop() {
  static unsigned long lastSend = 0;

  if (millis() - lastSend >= SEND_INTERVAL) {
    lastSend = millis();

    float distance = getDistanceCM();
    float temperature = bme.readTemperature();
    float humidity = bme.readHumidity();
    float pressure = bme.readPressure() / 100.0F; // Pa → hPa

    Serial.println("\n--- Sensor Data ---");
    Serial.printf("Distance: %.2f cm\n", distance);
    Serial.printf("Temperature: %.2f °C\n", temperature);
    Serial.printf("Humidity: %.2f %%\n", humidity);
    Serial.printf("Pressure: %.2f hPa\n", pressure);

    sendDataToGoogleSheet(distance, temperature, humidity, pressure);
  }
}
