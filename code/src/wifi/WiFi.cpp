#include "WiFi.h"

void setupWiFi() {
  WiFi.hostname(WIFI_HOSTNAME);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
}

String getWiFiStatus() {
  String ssid = WiFi.SSID();
  String hostname = WiFi.hostname();
  int rssi = WiFi.RSSI();
  String localIP = WiFi.localIP().toString();
  
  return "{\"network\":{\"wifi\":{\"ssid\":\"" + ssid + "\",\"hostname\":\"" + hostname + "\",\"rssi\":" + String(rssi) + ",\"localIP\":\"" + localIP + "\"}},\"general\":{\"uptime\":" + String(millis()) + ",\"batteryLevel\":" + String(85) + "}}";
}

String getWiFiInfo() {
  String ssid = WiFi.SSID();
  String hostname = WiFi.hostname();
  int rssi = WiFi.RSSI();
  String localIP = WiFi.localIP().toString();
  
  return "{\"ssid\":\"" + ssid + "\",\"hostname\":\"" + hostname + "\",\"rssi\":" + String(rssi) + ",\"localIP\":\"" + localIP + "\"}";
}

void setupOTA() {
  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH) {
      type = "sketch";
    } else { // U_SPIFFS
      type = "filesystem";
    }
    Serial.println("Start updating " + type);
  });

  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });

  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });

  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) {
      Serial.println("Auth Failed");
    } else if (error == OTA_BEGIN_ERROR) {
      Serial.println("Begin Failed");
    } else if (error == OTA_CONNECT_ERROR) {
      Serial.println("Connect Failed");
    } else if (error == OTA_RECEIVE_ERROR) {
      Serial.println("Receive Failed");
    } else if (error == OTA_END_ERROR) {
      Serial.println("End Failed");
    }
  });

  ArduinoOTA.begin();
  Serial.println("OTA ready");
}

void handleOTA() {
  ArduinoOTA.handle();
} 