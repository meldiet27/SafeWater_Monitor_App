#include "WiFiS3.h"
#include <ThingSpeak.h>
#include "VernierLib.h"

char ssid[] = "INSERT_WIFI_USER";        
char pass[] = "INSERT_WIFI_PASS";     

unsigned long myChannelNumber = INSERT_CHANNEL_ID;
const char * myWriteAPIKey = "INSERT_WRITE_API_KEY";

WiFiClient client;
VernierLib Vernier;

bool startMeasurement = false;  // flag controlled by serial

void setup() {
  Serial.begin(115200);
  while (!Serial);  // Wait for Serial USB

  WiFi.begin(ssid, pass);
  Serial.println("Connecting to WiFi...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected.");

  ThingSpeak.begin(client);
  Vernier.autoID();

  Serial.println("Waiting for start command from PC...");
}

void loop() {
  if (Serial.available() > 0) {
    String input = Serial.readStringUntil('\n');
    input.trim();
    if (input == "start") {
      startMeasurement = true;
      Serial.println("Start command received.");
    }
  }

  if (startMeasurement) {
    float sensorValue = Vernier.readSensor();
    Serial.print("Sensor Value: ");
    Serial.println(sensorValue);

    int statusCode = ThingSpeak.writeField(myChannelNumber, 1, sensorValue, myWriteAPIKey);

    if (statusCode == 200) {
      Serial.println("ThingSpeak update successful.");
    } else {
      Serial.print("Problem updating channel. HTTP error code: ");
      Serial.println(statusCode);
    }
    delay(20000);  // wait 20 seconds between uploads
  }
}

