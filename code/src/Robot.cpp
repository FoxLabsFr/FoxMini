#include "Robot.h"

#include <Wire.h>
#include <ESP8266WiFi.h>

Robot::Robot() : legs(), state(State::Initializing) {}

void Robot::init() {
  Serial.println("Initializing robot");
  state = State::Initializing;

  // Initialize with I2C address 0x40 and 8 servos
  legs.setIds(0x40, {1, 2, 3, 4, 5, 6, 7, 8});
  legs.setDefaultPosition({900, 900, 900, 900, 900, 900, 900, 900});

  // Optional config
  legs.setMinPulse({150, 150, 150, 150, 150, 150, 150, 150});
  legs.setMaxPulse({600, 600, 600, 600, 600, 600, 600, 600});
  legs.setOffsets({-137, 74, -117, -169, -6, 70, 22, 58});
  legs.setInverts({-1, 1, 1, -1, 1, -1, -1, 1});

  // Init servo at default position
  legs.init(Serial);

  // set initial position
  applyPresetPosition("init");
}

void Robot::update() {
  if (isWalking) {
    updateWalking();
  } 

  ServoGroup::State legsState = legs.getState();
  if (legsState == ServoGroup::State::IDLE) {
    if (state != State::Walking) {
      state = State::Idle;
    }
  } else if (legsState == ServoGroup::State::MOVING) {
    if (state != State::Walking) {
      state = State::Moving;
    }
  }
  legs.update();
}

void Robot::moveLegs(int16_t *positions, uint16_t duration) {
  legs.setPositions(positions, duration);
}

String Robot::getCurrentPositionName() { return currentPositionName; }

void Robot::applyPresetPosition(const String &name) {
  for (int i = 0; i < numPresetPositions; ++i) {
    if (presetPositions[i].name == name) {
      moveLegs(presetPositions[i].legsPositions, 0);
      currentPositionName = name;
      return;
    }
  }
  Serial.println("Preset position not found: " + name);
}

String *Robot::getAvailablePositionNames() {
  static String names[numPresetPositions];
  for (int i = 0; i < numPresetPositions; ++i) {
    names[i] = presetPositions[i].name;
  }
  return names;
}


Robot::IKResult Robot::calculateIK(float x, float y) {
  IKResult result;
  
  // Calculate the distance from origin to target
  float d = sqrt(x * x + y * y);
  
  // Check if the target is within reach
  if (d <= (segment1Length + segment2Length)) {
    // Calculate theta2 using law of cosines
    float cosTheta2 = (d * d - segment1Length * segment1Length - segment2Length * segment2Length) / (2 * segment1Length * segment2Length);
    float theta2 = acos(max(-1.0f, min(1.0f, cosTheta2)));
    
    // Calculate theta1
    float k1 = segment1Length + segment2Length * cos(theta2);
    float k2 = segment2Length * sin(theta2);
    float theta1 = atan2(y, x) - atan2(k2, k1);
    
    // Convert to degrees
    result.theta1 = theta1 * 180.0 / PI;
    result.theta2 = theta2 * 180.0 / PI;
    result.success = true;
  } else {
    // Target is out of reach - find the nearest reachable position
    float maxReach = segment1Length + segment2Length;
    
    // Calculate the angle to the target
    float targetAngle = atan2(y, x);
    
    // Calculate the nearest reachable point on the circle of maximum reach
    float nearestX = maxReach * cos(targetAngle);
    float nearestY = maxReach * sin(targetAngle);
    
    // Calculate the angles for this nearest reachable point
    float nearestD = maxReach;
    float cosTheta2 = (nearestD * nearestD - segment1Length * segment1Length - segment2Length * segment2Length) / (2 * segment1Length * segment2Length);
    float theta2 = acos(max(-1.0f, min(1.0f, cosTheta2)));
    
    float k1 = segment1Length + segment2Length * cos(theta2);
    float k2 = segment2Length * sin(theta2);
    float theta1 = atan2(nearestY, nearestX) - atan2(k2, k1);
    
    // Convert to degrees
    result.theta1 = theta1 * 180.0 / PI;
    result.theta2 = theta2 * 180.0 / PI;
    result.success = false;
  }
  
  // apply offset to the result
  result.theta1 += 0;
  result.theta2 += 90;

  return result;
}

void Robot::setRobotPose(float height, float pitch, float roll, uint16_t duration) {
  
  // Clamp height between 50 and 82 mm
  height = constrain(height, 50.0f, 82.0f);
  
  // Clamp pitch and roll angles to reasonable limits (±15 degrees)
  pitch = constrain(pitch, -15.0f, 15.0f);
  roll = constrain(roll, -15.0f, 15.0f);
  
  // Convert angles to radians
  float pitchRad = pitch * PI / 180.0f;
  float rollRad = roll * PI / 180.0f;
  
  // Robot body dimensions (distance from center to each leg)
  float bodyWidth = 60.0f;   // Distance between front/back legs
  float bodyLength = 80.0f;  // Distance between left/right legs
  
  // Calculate height offset for each leg based on orientation
  float frontLeftOffset = sin(pitchRad) * bodyWidth/2 + sin(rollRad) * bodyLength/2;
  float frontRightOffset = sin(pitchRad) * bodyWidth/2 - sin(rollRad) * bodyLength/2;
  float backLeftOffset = -sin(pitchRad) * bodyWidth/2 + sin(rollRad) * bodyLength/2;
  float backRightOffset = -sin(pitchRad) * bodyWidth/2 - sin(rollRad) * bodyLength/2;
  
  // Set each leg to appropriate height
  // Leg order: 0=FrontLeft, 1=FrontRight, 2=BackLeft, 3=BackRight
  float legHeights[4] = {
    height + frontLeftOffset,   // Front Left
    height + frontRightOffset,  // Front Right
    height + backLeftOffset,    // Back Left
    height + backRightOffset    // Back Right
  };
  
  // Calculate IK for all legs and set positions
  int16_t positions[8];
  for (int leg = 0; leg < 4; leg++) {
    float x = 0;  // Center position
    float y = legHeights[leg];  // Individual leg height    
    IKResult result = calculateIK(x, y);
    positions[leg] = result.theta1 * 10;  // First servo of the leg
    positions[leg + 4] = result.theta2 * 10;  // Second servo of the leg
  }
  
  // Move all legs to their calculated positions
  moveLegs(positions, duration);
}








void Robot::walk(int stepLength, int stepHeight, int stepDuration, int walkHeight, float xDirection, float yDirection, float xOffset) {
  
  // Clamp directions to -1.0 to 1.0 range
  xDirection = constrain(xDirection, -1.0f, 1.0f);
  yDirection = constrain(yDirection, -1.0f, 1.0f);
  
  // Only reset phase if not already walking or if parameters changed
  bool shouldResetPhase = !isWalking;
  
  // Initialize or update walking state
  isWalking = true;
  if (shouldResetPhase) {
    currentPhase = 0;
    phaseStartTime = millis();
  }
  
  walkStepLength = stepLength;
  walkStepHeight = stepHeight;
  walkStepDuration = stepDuration;
  this->walkHeight = walkHeight;
  walkXDirection = xDirection;
  walkYDirection = yDirection;
  walkXOffset = xOffset;
  
  state = State::Walking;
}

void Robot::stopWalking() {
  isWalking = false;
  state = State::Idle;
  
  // Return to stable standing position
  int16_t stablePositions[8];
  for (int i = 0; i < 8; i++) {
    stablePositions[i] = 900; // Center position for all servos
  }
  moveLegs(stablePositions, 500); // Smooth transition to stable position
}

void Robot::setWalkParameters(int stepLength, int stepHeight, int stepDuration, int walkHeight) {
  // Update all default walking parameters
  defaultStepLength = stepLength;
  defaultStepHeight = stepHeight;
  defaultStepDuration = stepDuration;
  defaultWalkHeight = walkHeight;
  
  Serial.println("Walk parameters updated:");
  Serial.print("  Step Length: "); Serial.println(defaultStepLength);
  Serial.print("  Step Height: "); Serial.println(defaultStepHeight);
  Serial.print("  Step Duration: "); Serial.println(defaultStepDuration);
  Serial.print("  Walk Height: "); Serial.println(defaultWalkHeight);
}

void Robot::updateWalking() {
  // Update phase timing
  unsigned long currentTime = millis();
  if (currentTime - phaseStartTime >= (unsigned long)walkStepDuration) {
    // Advance to next half-cycle (two-phase trot)
    currentPhase = (currentPhase + 1) % 2;
    phaseStartTime = currentTime;
  }
  // Compute phase progress [0..1] in the current half-cycle
  float phaseProgress = (float)(currentTime - phaseStartTime) / (float)walkStepDuration;

  float stepLength = walkStepLength;
  float stepHeight = walkStepHeight;
  float baseHeight = walkHeight;
  int16_t positions[8];

  // For each of the 4 legs (0=FL,1=FR,2=BL,3=BR)
  for (int leg = 0; leg < 4; leg++) {
    // Define trot pattern: legs 0&3 (diagonal) sync with phase=0, 1&2 sync with phase=0.5
    bool diag1 = (leg == 0 || leg == 3);
    float legPhase = diag1 ? 0.0f : 0.5f;
    float legProgress = phaseProgress + legPhase;
    if (legProgress >= 1.0f) legProgress -= 1.0f;

    float x, y;
    if (legProgress < 0.5f) {
      // SWING PHASE: foot lifts and moves forward
      float t = legProgress * 2.0f; // normalized swing [0..1]
      // Smooth horizontal motion: start at -L/2, end at +L/2 with cosine interpolation
      x = -stepLength/2.0f + (stepLength/2.0f) * (1 - cos(PI * t));
      // Vertical lift: sinusoidal for smooth up/down (zero at t=0, t=1)
      y = baseHeight + stepHeight * sin(PI * t);
    } else {
      // STANCE PHASE: foot stays on ground moving backward
      float s = (legProgress - 0.5f) * 2.0f; // normalized stance [0..1]
      // Smooth horizontal motion: start at +L/2, end at -L/2
      x = stepLength/2.0f - (stepLength/2.0f) * (1 - cos(PI * s));
      // Foot on ground
      y = baseHeight;
    }
    // Apply X offset
    x += walkXOffset;
    
    // Apply forward/backward direction
    x *= -walkYDirection;


    if(leg==0 || leg==1) {
      x=-x;
    }

    // Optional turning: scale step lengths for left vs right legs
    if (walkXDirection > 0.0f) { 
      // turning right: right-side legs (1,3) take smaller steps
      if (leg == 1 || leg == 3) x *= (1.0f - walkXDirection);
      else               x *= (1.0f + walkXDirection);
    } else if (walkXDirection < 0.0f) {
      // turning left: left-side legs (0,2) take smaller steps
      if (leg == 0 || leg == 2) x *= (1.0f + walkXDirection);
      else               x *= (1.0f - walkXDirection);
    }

    // Compute IK and set servo targets (convert to 10x position units as before)
    IKResult result = calculateIK(x, y);
    positions[leg] = (int16_t)(result.theta1 * 10);
    positions[leg + 4] = (int16_t)(result.theta2 * 10);
  }

  // Command servos (zero duration = immediate)
  moveLegs(positions, 0);
}






String Robot::getJson() {
  String json = "{";
  json += "\"servogroup\":";
  json += legs.getServoJson();
  json += ",\"state\":\"";
  switch (state) {
    case State::Idle:
      json += "idle";
      break;
    case State::Moving:
      json += "moving";
      break;
    case State::Sleep:
      json += "sleep";
      break;
    case State::Initializing:
      json += "initializing";
      break;
    case State::Walking:
      json += "walking";
      break;
  }
  json += "\",\"position\":\"";
  json += getCurrentPositionName();
  json += "\",\"rssi\":";
  json += String(WiFi.RSSI());
  json += "}";
  return json;
}
