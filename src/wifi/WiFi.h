#ifndef WIFI_H
#define WIFI_H

#include <ESP8266WiFi.h>
#include <ArduinoOTA.h>
#include "config/config.h"

// WiFi setup and management functions
void setupWiFi();

// WiFi status functions
String getWiFiStatus();
String getWiFiInfo();

// OTA (Over-The-Air) update functions
void setupOTA();
void handleOTA();

#endif // WIFI_H 