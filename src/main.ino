#include <Arduino.h>

#include "Robot.h"
#include "config/config.h"
#include "wifi/WiFi.h"

#if ENABLE_WEBAPI
  #include <webapi/WebAPI.h>
#endif

Robot robot;

void setup() {
  Serial.begin(115200);
  Serial.println("");
  Serial.println("Starting...");

  robot.init();
  delay(1000);

  setupWiFi();
  setupOTA();
  
  #if ENABLE_WEBAPI
    setupWebServer();
  #endif
}

void loop() {
  handleOTA();
  
  #if ENABLE_WEBAPI
    server.handleClient();
  #endif
  
  robot.update();
}