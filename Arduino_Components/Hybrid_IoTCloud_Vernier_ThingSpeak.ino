#include <WiFiS3.h>
#include <ArduinoIoTCloud.h>
#include <Arduino_ConnectionHandler.h>
#include <VernierLib.h>
#include <ThingSpeak.h>

// === WiFi Config ===
const char SSID[] = "INSERT_WIFI_USER";           // Replace with your WiFi SSID
const char PASS[] = "INSERT_WIFI_PASS";       // Replace with your WiFi password
WiFiConnectionHandler ArduinoIoTPreferredConnection(SSID, PASS);

VernierLib Vernier;

// === ThingSpeak Config ===
WiFiClient client;
unsigned long channelNumber = INSERT_CHANNEL_ID;
const char *writeAPIKey = "INSERT_WRITE_API_KEY";  // Replace with your ThingSpeak Write API key

// === Vernier Sensor
Vernier.autoID();

// === Measurement Logic
bool isMeasuring = false;
unsigned long lastUpload = 0;

// === HTTP Server
WiFiServer server(80);

void initProperties() {
  // No cloud variables used
}

void setup() {
  Serial.begin(9600);
  delay(1500);

  initProperties();
  ArduinoCloud.begin(ArduinoIoTPreferredConnection);

  // Optional: Debug messages
  setDebugMessageLevel(2);
  ArduinoCloud.printDebugInfo();

  vaSensor.begin();         // Init Vernier shield
  server.begin();           // Start HTTP server
  ThingSpeak.begin(client); // Init ThingSpeak
}

void loop() {
  ArduinoCloud.update(); // Maintain WiFi connection

  // Handle /start and /stop from Python GUI
  WiFiClient remoteClient = server.available();
  if (remoteClient) {
    String request = remoteClient.readStringUntil('\r');
    remoteClient.flush();

    if (request.indexOf("/start") != -1) {
      isMeasuring = true;
      remoteClient.println("HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\n\r\nMeasurement Started");
      Serial.println("Measurement started via GUI.");
    } else if (request.indexOf("/stop") != -1) {
      isMeasuring = false;
      remoteClient.println("HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\n\r\nMeasurement Stopped");
      Serial.println("Measurement stopped via GUI.");
    } else {
      remoteClient.println("HTTP/1.1 404 Not Found\r\nContent-Type: text/plain\r\n\r\nUnknown Endpoint");
    }

    delay(1);
    remoteClient.stop();
  }

  // Measurement logic
  if (isMeasuring && millis() - lastUpload >= 20000) {
    float value = vaSensor.readCalibratedValue();  // Calibrated reading from Vernier
    Serial.print("Vernier Sensor Reading: ");
    Serial.println(value);

    ThingSpeak.setField(1, value);
    int status = ThingSpeak.writeFields(channelNumber, writeAPIKey);

    if (status == 200) {
      Serial.println("Upload to ThingSpeak successful.");
    } else {
      Serial.print("ThingSpeak upload failed. HTTP error code: ");
      Serial.println(status);
    }

    lastUpload = millis();
  }
}

