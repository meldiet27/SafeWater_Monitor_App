#include <WiFiS3.h>
#include <WiFiClient.h>
#include <WiFiServer.h>
#include <ThingSpeak.h>
#include <VernierLib.h>

const char* ssid = "INSERT_WIFI_NAME";
const char* password = "INSERT_WIFI_PASSWORD";

WiFiServer server(80);
WiFiClient client;
VernierLib Vernier;

unsigned long myChannelNumber = INSERT_CHANNEL_NUMBER;
const char* myWriteAPIKey = "INSERT_WRITE_API_KEY";

bool isMeasuring = false;
unsigned long lastUpload = 0;
const unsigned long uploadInterval = 20000;

void setup() {
  Serial.begin(111520);
  while (!Serial);

  Serial.println(" Starting WiFi Sensor Board (R4 only)");

  WiFi.begin(ssid, password);
  int tries = 0;
  while (WiFi.status() != WL_CONNECTED && tries < 20) {
    delay(500);
    Serial.print(".");
    tries++;
  }

  if (WiFi.status() == WL_CONNECTED && WiFi.localIP() != IPAddress(0, 0, 0, 0)) {
    Serial.println("\n✅ WiFi connected (DHCP)");
    Serial.print(" IP: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\n⚠️ DHCP failed — using fallback static IP...");
    IPAddress ip(172, 20, 10, 3);
    IPAddress gateway(172, 20, 10, 1);
    IPAddress subnet(255, 255, 255, 240);
    WiFi.config(ip, gateway, subnet);
    WiFi.begin(ssid, password);
    delay(3000);
    Serial.print("Fallback IP: ");
    Serial.println(WiFi.localIP());
  }

  server.begin();
  ThingSpeak.begin(client);
  Vernier.autoID();
  Serial.println("Testing ThingSpeak upload...");
  int test = ThingSpeak.writeField(myChannelNumber, 1, 42.0f, myWriteAPIKey);
  Serial.print(" Test upload result: ");
  Serial.println(test);

}

void loop() {
  WiFiClient incoming = server.available();
  if (incoming) {
    Serial.println("\n Client connected");
    String request = "";
    unsigned long timeout = millis() + 2000;
    while (incoming.connected() && millis() < timeout) {
      while (incoming.available()) {
        char c = incoming.read();
        request += c;
      }
    }

    Serial.print(" Request: ");
    Serial.println(request);

    String response = "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nConnection: close\r\n\r\n";

    if (request.indexOf("GET /start") >= 0) {
      isMeasuring = true;
      response += "Measurement started.";
      Serial.println(" Measurement started.");
    } else if (request.indexOf("GET /stop") >= 0) {
      isMeasuring = false;
      response += "Measurement stopped.";
      Serial.println("Measurement stopped.");
    } else {
      response += "SafeWater R4 Sensor is online.";
    }

    incoming.print(response);
    delay(100);
    incoming.stop();
    Serial.println(" Client disconnected");
  }

  if (isMeasuring && millis() - lastUpload >= uploadInterval) {
    float E = Vernier.readSensor();  // analog pin A1 by default
    float finalResult = pow(10, (E - 1.0) / 26.0);
    int x = ThingSpeak.writeField(myChannelNumber, 1, finalResult, myWriteAPIKey);
    if (x == 200) {
      Serial.print(" Successfully uploaded data to ThingSpeak: ");
      Serial.println(finalResult);
    } else {
      Serial.print(" Error uploading data to ThingSpeak: ");
      Serial.println(x);
    }
    lastUpload = millis();
  }
}
