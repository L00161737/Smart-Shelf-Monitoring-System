#include <WiFiNINA.h>
#include "secrets.h"
#include "ThingSpeak.h" // Always include ThingSpeak header file after other headers

// WiFi credentials
char ssid[] = SECRET_SSID;    // Your network SSID (name)
char pass[] = SECRET_PASS;    // Your network password
WiFiClient client;

// ThingSpeak details
unsigned long myChannelNumber = SECRET_CH_ID;
const char *myWriteAPIKey = SECRET_WRITE_APIKEY;

// Sensor pins
int sensor1 = 14;
int sensor2 = 15;
int sensor3 = 16;
int sensor4 = 17;
int sensor5 = 20;
int sensor6 = 21;

// Optional LED indicators for stock status
#define LOW_STOCK_LED 8
#define NORMAL_STOCK_LED 9

void setup() {
  Serial.begin(9600);

  // Set sensor pins as input
  pinMode(sensor1, INPUT);
  pinMode(sensor2, INPUT);
  pinMode(sensor3, INPUT);
  pinMode(sensor4, INPUT);
  pinMode(sensor5, INPUT);
  pinMode(sensor6, INPUT);

  // Set LED pins as output (if using LED indicators)
  pinMode(LOW_STOCK_LED, OUTPUT);
  pinMode(NORMAL_STOCK_LED, OUTPUT);

  // Check for WiFi module
  if (WiFi.status() == WL_NO_MODULE) {
    Serial.println("Communication with WiFi module failed!");
    while (true); // Halt execution
  }

  // Check WiFi firmware version
  String fv = WiFi.firmwareVersion();
  if (fv != "1.0.0") {
    Serial.println("Please upgrade the firmware");
  }

  // Connect to WiFi
  ensureWiFiConnected();

  // Initialize ThingSpeak
  ThingSpeak.begin(client);
}

// Function to ensure WiFi remains connected with retry limit
void ensureWiFiConnected() {
  int retryCount = 0;
  int maxRetries = 5; // Set retry limit

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi disconnected! Attempting to reconnect...");
    WiFi.disconnect(); // Clear failed connection attempts

    while (WiFi.status() != WL_CONNECTED && retryCount < maxRetries) {
      WiFi.begin(ssid, pass);
      delay(5000);
      retryCount++;
      Serial.print("Retry attempt: ");
      Serial.println(retryCount);
    }

    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("Reconnected to WiFi.");
    } else {
      Serial.println("Failed to reconnect to WiFi. Retrying in next cycle.");
    }
  }
}

// Function to count number of 1s in binary representation of counter
int countOnes(int num) {
  int count = 0;
  while (num > 0) {
    count += num & 1;  // Add 1 if the last bit is 1
    num >>= 1;         // Shift bits to the right
  }
  return count;
}

void loop() {
  ensureWiFiConnected(); // Ensure WiFi stays connected

  // Read sensor values
  int s1 = digitalRead(sensor1);
  int s2 = digitalRead(sensor2);
  int s3 = digitalRead(sensor3);
  int s4 = digitalRead(sensor4);
  int s5 = digitalRead(sensor5);
  int s6 = digitalRead(sensor6);

  // Print sensor values for debugging
  Serial.print("Sensor Readings: ");
  Serial.print(s1); Serial.print(" ");
  Serial.print(s2); Serial.print(" ");
  Serial.print(s3); Serial.print(" ");
  Serial.print(s4); Serial.print(" ");
  Serial.print(s5); Serial.print(" ");
  Serial.println(s6);

  // Calculate stock count using binary encoding
  int counter = s6 * 32 + s5 * 16 + s4 * 8 + s3 * 4 + s2 * 2 + s1 * 1;

  // Convert binary counter to the number of 1s
  int stockCount = countOnes(counter);

  Serial.print("Binary Value: ");
  Serial.println(counter, BIN);  // Print binary representation
  Serial.print("Calculated Stock Count (Sent to ThingSpeak): ");
  Serial.println(stockCount);
  Serial.print("Decimal Value (For Debugging): ");
  Serial.println(counter);

  // Update LED indicators based on stock levels
  if (stockCount < 2) {
    digitalWrite(LOW_STOCK_LED, HIGH);
    digitalWrite(NORMAL_STOCK_LED, LOW);
    Serial.println("Stock is LOW! LED Alert ON.");
  } else {
    digitalWrite(LOW_STOCK_LED, LOW);
    digitalWrite(NORMAL_STOCK_LED, HIGH);
    Serial.println("Stock is OK.");
  }

  // Write data to ThingSpeak
  ThingSpeak.setField(1, stockCount);  // Field 1: actual stock count (0–6)
  ThingSpeak.setField(2, counter);     // Field 2: binary decimal value

  int response = ThingSpeak.writeFields(myChannelNumber, myWriteAPIKey);

  if (response == 200) {
    Serial.println("Channel update successful.");
  } else {
    Serial.print("Problem updating channel. HTTP error code: ");
    Serial.println(response);
  }

  delay(20000); // Wait 20 seconds before updating again
}

