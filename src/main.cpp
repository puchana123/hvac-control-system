#include <Arduino.h>
#include <DHT.h>
#include <DHT_U.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include "config.h" // Configuration file for WiFi & MQTT


// Pins definition
#define DHT_PIN 15
#define DHT_TYPE DHT22
const int LED_PIN = 12;
const int BUTTON_PIN = 14;

// Control Objects
DHT dht(DHT_PIN, DHT_TYPE);
LiquidCrystal_I2C lcd(0x27, 16, 2);

// Timing
unsigned long lastRead = 0;
const unsigned long INTERVAL = 2000; // 2 seconds
unsigned long buttonPressedTime = 0;
const int LONG_PRESS_DURATION = 1000; // 1 seconds
unsigned long lastClickTime = 0;
int clickCount = 0;
const int DOUBLE_CLICK_DURATION = 500; // 0.5 seconds

// State
bool isOn = false;
bool isButtonPressed = false;
enum SystemMode
{
  AUTO,
  MANUAL
};

SystemMode currentMode = AUTO;

// Temperature
const float TEMP_HIGH = 30.0;
const float TEMP_LOW = 28.0;

// Blinking
unsigned long lastBlink = 0;
bool isBlackLightOn = false;
int blinkCount = 0;
int maxBlinks = 0;
bool isBlinking = false;

WiFiClientSecure espClient;
PubSubClient client(espClient);

void setup_wifi()
{
  delay(10);
  lcd.clear();
  lcd.print("WiFi Connecting...");
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    lcd.print(".");
  }
  lcd.clear();
  lcd.print("WiFi Connected!");
  espClient.setInsecure();
}

void reconnect() {
  while (!client.connected())
  {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect with client id "ESP32Client01" and password on config file
    if (client.connect(CLIENT_ID, MQTT_USER, MQTT_PASS)) 
    {
      client.subscribe(TOPIC_CONTROL);
      Serial.println("connected");
      lcd.setCursor(0, 1);
      lcd.print("MQTT Online");
    }
    else
    {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      delay(5000);
    }
  }
}

void startBlink(int times)
{
  isBlinking = true;
  maxBlinks = times * 2;
  blinkCount = 0;
  lastBlink = millis();
}

void handleBacklightBlink() {
  if (!isBlinking) return; // If not blinking mode, do nothing

  if(millis() - lastBlink >= 150){
    lastBlink = millis();
    isBlackLightOn = !isBlackLightOn;
    
    if (isBlackLightOn){
      lcd.noBacklight();
    } else {
      lcd.backlight();
    }

    blinkCount++;
    // Stop blinking once the maximum number of blinks is reached
    if (blinkCount >= maxBlinks) {
      isBlinking = false;
      lcd.backlight();
    }
  }
}

void checkTemp(float currentTemp)
{
  if (currentTemp >= TEMP_HIGH && !isOn)
  {
    isOn = true;
    startBlink(2);
  }
  else if (currentTemp <= TEMP_LOW)
  {
    isOn = false;
  }
  digitalWrite(LED_PIN, isOn ? HIGH : LOW);
}

void updateDisplay(float temp)
{
  lcd.setCursor(0, 0);
  if (currentMode == AUTO)
  {
    lcd.print("Auto   ");
  }
  else
  {
    lcd.print("Manual ");
  }
  lcd.print("|Fan: ");
  lcd.print(isOn ? "ON " : "OFF");
  lcd.setCursor(0, 1);
  lcd.print("Temp: ");
  if (isnan(temp))
  {
    lcd.print("--.-");
  }
  else
  {
    lcd.print(temp, 1);
  }
  lcd.print(" C    ");
}

void checkButtonState()
{
  bool currentButtonState = digitalRead(BUTTON_PIN);
  // Check for button press
  if (currentButtonState == LOW && !isButtonPressed)
  {
    buttonPressedTime = millis(); // Record the time when the button is pressed
    isButtonPressed = true;
  }
  // Check for button release
  if (currentButtonState == HIGH && isButtonPressed)
  {
    unsigned long now = millis();
    unsigned long pressDuration = now - buttonPressedTime;

    if (pressDuration > 50)
    {
      // double click detected, switch to mode
      if (now - lastClickTime < DOUBLE_CLICK_DURATION)
      {
        currentMode = (currentMode == AUTO) ? MANUAL : AUTO;
        startBlink(3);
        Serial.printf("Switching to [%s] mode.\r\n", (currentMode == AUTO ? "AUTO" : "MANUAL"));
        clickCount = 0;
      }
      else
      {
        // Short press detected, switch fan mode
        if (currentMode == MANUAL)
        {
          isOn = !isOn;
          digitalWrite(LED_PIN, isOn ? HIGH : LOW);
          Serial.printf("Manual Toggle: %s\r\n", isOn ? "ON" : "OFF");
        }
      }
      lastClickTime = now;
      updateDisplay(dht.readTemperature());
    }
    isButtonPressed = false;
  }
}

void autoTest()
{
  lcd.print("Auto Testing...");
  delay(2000);
  lcd.clear();
  Serial.println("--- Starting Auto Test ---");
  for (float t = 25.0; t <= 32.0; t += 0.5)
  {
    checkTemp(t);
    updateDisplay(t);
    Serial.printf("Temperature: %.1f C | Fan: %s \r\n", t, isOn ? "ON" : "OFF");
    delay(500);
  }
  for (float t = 32.0; t >= 25.0; t -= 0.5)
  {
    checkTemp(t);
    updateDisplay(t);
    Serial.printf("Temperature: %.1f C | Fan: %s \r\n", t, isOn ? "ON" : "OFF");
    delay(500);
  }
  Serial.println("--- Auto Test Finished ---");
  delay(2000);
  lcd.clear();
  lcd.print("System Ready!");
  delay(2000);
  lcd.clear();
  delay(2000);
  lcd.clear();
}

void callback(char* topic, byte* payload, unsigned int length) {
  // Handle incoming MQTT messages
  StaticJsonDocument<128> doc;
  deserializeJson(doc, payload, length);
  const char* cmd = doc["cmd"];

  if (strcmp(cmd, "TOGGLE_MODE") == 0) {
    currentMode = (currentMode == AUTO) ? MANUAL : AUTO;
    startBlink(2);
  } else if (currentMode == MANUAL) {
    if (strcmp(cmd, "FAN_ON") == 0) {
      isOn = true;
    } else if (strcmp(cmd, "FAN_OFF") == 0) {
      isOn = false;
    }
    digitalWrite(LED_PIN, isOn ? HIGH : LOW);
  }
}

void setup()
{
  Serial.begin(115200);
  dht.begin();
  pinMode(LED_PIN, OUTPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("HVAC System");
  
  setup_wifi();
  client.setServer(MQTT_BROKER, MQTT_PORT);
  client.setCallback(callback);
  delay(1000);
  lcd.clear();

  lcd.setCursor(0, 1);
  lcd.print("Starting...");
  delay(2000);
  lcd.clear();

  autoTest();
}

void loop()
{
  if (!client.connected())
  {
    reconnect();
  }
  client.loop();


  handleBacklightBlink();
  checkButtonState();

  // Read temperature every 2 seconds
  unsigned long currentMillis = millis();

  if (currentMillis - lastRead >= INTERVAL)
  {
    lastRead = currentMillis;
    float temp = dht.readTemperature();
    // Check if any reads failed and exit early (to try again).
    if (isnan(temp))
    {
      Serial.println("Failed to read from DHT sensor!");
      lcd.setCursor(0, 0);
      lcd.print("Sensor Error!!!   ");
      return;
    }
    if (currentMode == AUTO)
    {
      checkTemp(temp);
    }
    updateDisplay(temp);
    Serial.printf("Temperature: %.1f C | Fan: %s \r\n", temp, isOn ? "ON" : "OFF");

    // Publish temperature and fan state to MQTT
    StaticJsonDocument<128> doc;
    doc["temp"] = temp;
    doc["mode"] = (currentMode == AUTO? "AUTO" : "MANUAL");
    doc["fan"] = isOn ? 1 : 0;
    char buffer[128];
    serializeJson(doc, buffer);
    client.publish(TOPIC_STATUS, buffer);
  }
}