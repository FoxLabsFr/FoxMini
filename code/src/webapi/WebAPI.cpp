#include "WebAPI.h"
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <FS.h>
#include "config/config.h"
#include "wifi/WiFi.h"

#if ENABLE_WEBAPI

ESP8266WebServer server(80);

// Handler prototypes
void handleSetPositions();
void handleGetRobot();
void handleInverseKinematics();
void handleWalkStart();
void handleWalkStop();
void handleWalkControl();
void handleApplyPresetPosition();
void handleSetRobotPose();
void handleRoot();
void handleStaticFiles();

void setupWebServer() {
  // Initialize SPIFFS
  if (!SPIFFS.begin()) {
    Serial.println("SPIFFS initialization failed!");
    return;
  }
  Serial.println("SPIFFS initialized successfully");

  // Add CORS headers to all responses
  server.onNotFound([]() {
    server.sendHeader("Access-Control-Allow-Origin", "*");
    server.sendHeader("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
    server.sendHeader("Access-Control-Allow-Headers", "Content-Type");
    server.send(404, "text/plain", "Not found");
  });
  
  // Handle preflight OPTIONS requests
  server.on("/api", HTTP_OPTIONS, []() {
    server.sendHeader("Access-Control-Allow-Origin", "*");
    server.sendHeader("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
    server.sendHeader("Access-Control-Allow-Headers", "Content-Type");
    server.send(200);
  });
  
  // Root path - serve the main HTML file
  server.on("/", HTTP_GET, handleRoot);
  
  // Static files handler
  server.on("/styles.css", HTTP_GET, handleStaticFiles);
  server.on("/scripts.js", HTTP_GET, handleStaticFiles);
  
  // API endpoint for all robot control
  server.on("/api", HTTP_GET, handleAPI);
  
  server.begin();
  Serial.println("HTTP server started");
}

void handleRoot() {
  File file = SPIFFS.open("/index.html", "r");
  if (!file) {
    server.send(404, "text/plain", "File not found");
    return;
  }
  server.streamFile(file, "text/html");
  file.close();
}

void handleStaticFiles() {
  String path = server.uri();
  String contentType = "text/plain";
  
  if (path.endsWith(".css")) {
    contentType = "text/css";
  } else if (path.endsWith(".js")) {
    contentType = "application/javascript";
  }
  
  File file = SPIFFS.open(path, "r");
  if (!file) {
    server.send(404, "text/plain", "File not found");
    return;
  }
  server.streamFile(file, contentType);
  file.close();
}

void handleAPI() {
  String action = server.hasArg("action") ? server.arg("action") : "";
  
  if (action == "getRobot") {
    handleGetRobot();
  } else if (action == "setPositions") {
    handleSetPositions();

  } else if (action == "walkStart") {
    handleWalkStart();
  } else if (action == "walkStop") {
    handleWalkStop();
  } else if (action == "walkControl") {
    handleWalkControl();
  } else if (action == "applyPresetPosition") {
    handleApplyPresetPosition();
  } else if (action == "setRobotPose") {
    handleSetRobotPose();
  } else if (action == "inverseKinematics") {
    handleInverseKinematics();
  } else if (action == "update") {
    handleUpdate();
  } else {
    server.sendHeader("Access-Control-Allow-Origin", "*");
    server.send(400, "application/json", "{\"error\":\"Unknown action: " + action + "\"}");
  }
}

void handleUpdate() {}

void initializePositions(int16_t* positions, int count) {
  for (int i = 0; i < count; ++i) {
    positions[i] = -1;
  }
}

void handleSetPositions() {
  server.sendHeader("Access-Control-Allow-Origin", "*");
  if (server.hasArg("arm") && server.hasArg("servo") &&
      server.hasArg("angle")) {
    String arm = server.arg("arm");
    int servo = server.arg("servo").toInt();
    int angle = server.arg("angle").toInt();

    if (arm == "legs" && servo >= 0 && servo < robot.legs.num_servos) {
      int16_t positions[robot.legs.num_servos];
      initializePositions(positions, robot.legs.num_servos);
      positions[servo] = angle;
      robot.moveLegs(positions, 500);
      
      String json = "{\"status\":\"position_set\",\"arm\":\"" + arm + "\",\"servo\":" + String(servo) + ",\"angle\":" + String(angle) + "}";
      server.send(200, "application/json", json);
    } else {
      server.send(400, "application/json", "{\"error\":\"Invalid servo index\"}");
    }
  } else {
    server.send(400, "application/json", "{\"error\":\"Missing arm, servo, or angle parameter\"}");
  }
}

void handleGetRobot() {
  server.sendHeader("Access-Control-Allow-Origin", "*");
  String json = robot.getJson();
  server.send(200, "application/json", json);
}



void handleInverseKinematics() {
  server.sendHeader("Access-Control-Allow-Origin", "*");
  if (server.hasArg("x") && server.hasArg("y")) {
    float x = server.arg("x").toFloat();
    float y = server.arg("y").toFloat();
    
    // Use the Robot class to calculate inverse kinematics
    Robot::IKResult result = robot.calculateIK(x, y);
    

      // Convert angles to servo positions (multiply by 10 for servo range)
      int16_t servo1 = (int16_t)(result.theta1 * 10); // First servo angle
      int16_t servo2 = (int16_t)(result.theta2 * 10); // Second servo angle
      
      // Move all legs to the same position
      int16_t positions[robot.legs.num_servos];
      for (int leg = 0; leg < 4; leg++) {
        positions[leg] = servo1;     // First servo of each leg
        positions[leg+ 4] = servo2; // Second servo of each leg
      }
      
      // Move the legs
      robot.moveLegs(positions, 500);

    
    // Create JSON response based on the result
    String json = "{\"theta1\":";
    json += String(result.theta1);
    json += ",\"theta2\":";
    json += String(result.theta2);
    json += ",\"success\":";
    json += result.success ? "true" : "false";
    json += "}";
    
    // Always return the result, even if the target was out of reach
    // The client can check the success flag to determine if the target was reached
    server.send(200, "application/json", json);
  } else {
    server.send(400, "application/json", "{\"error\":\"Missing x or y parameter\"}");
  }
}

void handleWalkStart() {
  server.sendHeader("Access-Control-Allow-Origin", "*");

  // Start walking forward with default parameters
  robot.walk(70, 10, 400, 65, 1.0f, 0.0f);
  
  // Return success response
  String json = "{\"status\":\"walking_started\"}";  
  server.send(200, "application/json", json);
}

void handleWalkStop() {
  server.sendHeader("Access-Control-Allow-Origin", "*");
  // Stop walking
  robot.stopWalking();
  
  // Return success response
  String json = "{\"status\":\"walking_stopped\"}";
  server.send(200, "application/json", json);
}

void handleWalkControl() {
  server.sendHeader("Access-Control-Allow-Origin", "*");
  
  if (server.hasArg("x") && server.hasArg("y")) {
    float xDirection = server.arg("x").toFloat();
    float yDirection = server.arg("y").toFloat();
    
    // Check if both directions are zero (stop walking)
    if (abs(xDirection) < 0.1f && abs(yDirection) < 0.1f) {
      // Stop walking
      robot.stopWalking();
      String json = "{\"status\":\"walking_stopped\"}";
      server.send(200, "application/json", json);
      return;
    }

    // Get walking parameters from UI, with defaults if not provided
    int stepLength = server.hasArg("stepLength") ? server.arg("stepLength").toInt() : 70;
    int stepHeight = server.hasArg("stepHeight") ? server.arg("stepHeight").toInt() : 10;
    int stepDuration = server.hasArg("stepDuration") ? server.arg("stepDuration").toInt() : 400;
    int walkHeight = server.hasArg("walkHeight") ? server.arg("walkHeight").toInt() : 65;
    float xOffset = server.hasArg("xOffset") ? server.arg("xOffset").toFloat() : 0.0f;

    // Use walk method with all parameters from UI
    robot.walk(stepLength, stepHeight, stepDuration, walkHeight, xDirection, yDirection, xOffset);
    
    // Return success response with the parameters used
    String json = "{\"status\":\"walking_controlled\"";
    json += ",\"stepLength\":" + String(stepLength);
    json += ",\"stepHeight\":" + String(stepHeight);
    json += ",\"stepDuration\":" + String(stepDuration);
    json += ",\"walkHeight\":" + String(walkHeight);
    json += ",\"xDirection\":" + String(xDirection);
    json += ",\"yDirection\":" + String(yDirection);
    json += ",\"xOffset\":" + String(xOffset) + "}";
    
    server.send(200, "application/json", json);
  } else {
    server.send(400, "application/json", "{\"error\":\"Missing x or y parameter\"}");
  }
}

void handleApplyPresetPosition() {
  server.sendHeader("Access-Control-Allow-Origin", "*");
  
  if (server.hasArg("name")) {
    String presetName = server.arg("name");
    
    // Apply the preset position using the Robot class
    robot.applyPresetPosition(presetName);
    
    // Return success response
    String json = "{\"status\":\"preset_applied\",\"name\":\"" + presetName + "\"}";
    server.send(200, "application/json", json);
  } else {
    server.send(400, "application/json", "{\"error\":\"Missing name parameter\"}");
  }
}

void handleSetRobotPose() {
  server.sendHeader("Access-Control-Allow-Origin", "*");
  if (server.hasArg("height") && server.hasArg("pitch") && server.hasArg("roll")) {
    float height = server.arg("height").toFloat();
    float pitch = server.arg("pitch").toFloat();
    float roll = server.arg("roll").toFloat();
    
    // Set robot pose using the Robot class with fixed duration
    robot.setRobotPose(height, pitch, roll, 100);
    
    // Return success response
    String json = "{\"status\":\"pose_set\",\"height\":" + String(height);
    json += ",\"pitch\":" + String(pitch);
    json += ",\"roll\":" + String(roll) + "}";
    
    server.send(200, "application/json", json);
  } else {
    server.send(400, "application/json", "{\"error\":\"Missing height, pitch, or roll parameter\"}");
  }
}

#endif // ENABLE_WEBAPI