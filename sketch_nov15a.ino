// --- Use this file in Arduino IDE ---

#include <WiFi.h>
#include <ThingSpeak.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// --- WI-FI & THINGSPEAK SETTINGS ---
const char* ssid = " ";          // Use a 2.4GHz Wi-Fi network
const char* password = " ";

unsigned long channelNumber = ; // Example: 2748192 (no quotes)
const char* writeAPIKey = " ";

// --- PIN MAPPING (ESP32-C3 SUPER MINI) ---
#define OLED_SDA 8
#define OLED_SCK 9
#define REED_PIN 0
#define PIR_PIN 1
#define MQ_PIN 2

// --- OLED SETTINGS ---
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

WiFiClient client;

int gasLevel = 0;
int motionDetected = 0;
int doorOpen = 0;

unsigned long lastUploadTime = 0;
const unsigned long uploadInterval = 20000; // 20 seconds (respects 15s free limit)

void connectWiFi() {
  if (WiFi.status() == WL_CONNECTED) return;

  Serial.print("Connecting to WiFi: ");
  Serial.println(ssid);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  unsigned long startAttempt = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - startAttempt < 15000) {
    delay(500);
    Serial.print(".");
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi connected! IP: " + WiFi.localIP().toString());
  } else {
    Serial.println("\nWiFi connection failed. Check 2.4GHz SSID and password.");
  }
}

void updateOLED() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  display.setCursor(0, 0);
  display.println("- CYBERNERD TS -");

  display.setCursor(0, 18);
  display.print("Gas: ");
  display.println(gasLevel);

  display.setCursor(0, 32);
  display.print("Motion: ");
  display.println(motionDetected ? "DETECTED!" : "Clear");

  display.setCursor(0, 46);
  display.print("Door: ");
  display.println(doorOpen ? "OPEN" : "Closed");

  display.setCursor(0, 56);
  display.print("WiFi: ");
  display.println(WiFi.status() == WL_CONNECTED ? "OK" : "OFF");

  display.display();
}

void setup() {
  Serial.begin(115200);
  delay(1500);

  pinMode(PIR_PIN, INPUT);
  pinMode(REED_PIN, INPUT);
  pinMode(MQ_PIN, INPUT);

  Wire.begin(OLED_SDA, OLED_SCK);
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("OLED not found on SDA=8, SCK=9");
  } else {
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 20);
    display.println("Starting CYBERNERD...");
    display.display();
  }

  connectWiFi();
  ThingSpeak.begin(client);
}

void loop() {
  connectWiFi();

  gasLevel = analogRead(MQ_PIN);
  motionDetected = digitalRead(PIR_PIN) == HIGH ? 1 : 0;
  doorOpen = digitalRead(REED_PIN) == HIGH ? 1 : 0;

  updateOLED();

  if (WiFi.status() == WL_CONNECTED && (millis() - lastUploadTime >= uploadInterval)) {
    lastUploadTime = millis();

    ThingSpeak.setField(1, gasLevel);
    ThingSpeak.setField(2, motionDetected);
    ThingSpeak.setField(3, doorOpen);

    int statusCode = ThingSpeak.writeFields(channelNumber, writeAPIKey);
    if (statusCode == 200) {
      Serial.println("ThingSpeak update successful! HTTP 200");
    } else {
      Serial.print("ThingSpeak error. HTTP code: ");
      Serial.println(statusCode);
    }
  }

  delay(500);
}
