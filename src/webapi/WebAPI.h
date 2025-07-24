#ifndef WEBAPI_H
#define WEBAPI_H

#include <ESP8266WebServer.h>
#include <Arduino.h>
#include "Robot.h"
#include "config/config.h"

#if ENABLE_WEBAPI
  #if ENABLE_WEBSOCKET
    #include "WebSocket/WebSocket.h"
  #endif

  extern ESP8266WebServer server;
  extern Robot robot;

  // Function declarations for web interface
  void setupWebServer();
  void handleAPI();
  void handleRoot();
  void handleStaticFiles();
  void handleUpdate();
  void initializePositions(int16_t* positions, int count);
  void handleSetPositions();
  void handleGetRobot();
  void handleInverseKinematics();
  void handleWalkStart();
  void handleWalkStop();
  void handleWalkControl();
  void handleApplyPresetPosition();
  void handleSetIMUZero();
  void handleCalibrateIMU();
  void handleToggleDriftCompensation();
  void handleResetDriftCompensation();
  void handleSetRobotPose();
#endif

#endif  // WEBAPI_H
