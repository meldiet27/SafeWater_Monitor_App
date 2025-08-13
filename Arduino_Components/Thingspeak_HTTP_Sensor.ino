#include <WiFiS3.h>
#include "ThingSpeak.h"
#include "VernierLib.h"
#include <WebServer.h>
#include <math.h>


char ssid[] = "INSERT_WIFI_NAME";         // your network SSID (name)
char pass[] = "INSERT_WIFI_PASS";     // your network password

int keyIndex = 0;                  // your network key Index number (needed only for WEP)

WiFiClient client;
WebServer server(80);
VernierLib Vernier;

unsigned long myChannelNumber = 2877004; // Replace with your ThingSpeak channel number
const char * myWriteAPIKey = "WJ920IVANQFOGP30"; // Replace with your ThingSpeak Write API Key

unsigned long lastUploadTime = 0;
bool isMeasuring = false;

void setup() {
  Serial.begin(115200);
  while (!Serial);

  WiFi.begin(ssid, pass);
  Serial.print("Connecting to WiFi");

  int retries = 0;
  while (WiFi.status() != WL_CONNECTED && retries < 20) {
    delay(500);
    Serial.print(".");
    retries++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n✅ WiFi connected!");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\n❌ Failed to connect to WiFi.");
    return;
  }

  ThingSpeak.begin(client);
  Vernier.autoID();

  // Add HTTP GET endpoint for live reading
  server.on("/read", HTTP_GET, []() {
    float E = Vernier.readSensor();
    float finalResult = pow(10, (E - 1.0) / 26.0);
    server.send(200, "text/plain", String(finalResult));
  });

    // Add /start endpoint to begin measurement
  server.on("/start", HTTP_GET, []() {
    isMeasuring = true;
    server.send(200, "text/plain", "✅ Measurement started");
  });

  // Add /stop endpoint to stop measurement
  server.on("/stop", HTTP_GET, []() {
    isMeasuring = false;
    server.send(200, "text/plain", "🛑 Measurement stopped");
  });

  server.begin();
  Serial.println("🚀 Web server started.");
}

void loop() {
  server.handleClient();

  // Upload to ThingSpeak every 20 seconds
  if (millis() - lastUploadTime >= 20000) {
    float E = Vernier.readSensor();
    float finalResult = pow(10, (E - 1.0) / 26.0);
    int response = ThingSpeak.writeField(myChannelNumber, 1, finalResult, myWriteAPIKey);
    if (response == 200) {
      Serial.println("✅ Uploaded to ThingSpeak!");
    } else {
      Serial.print("⚠️ ThingSpeak error: ");
      Serial.println(response);
    }
    lastUploadTime = millis();
  }
}

