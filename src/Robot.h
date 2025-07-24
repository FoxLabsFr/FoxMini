#ifndef Robot_H
#define Robot_H


#include <ServoGroup.h>

class Robot {
 public:
  enum class State { Idle, Moving, Sleep, Initializing, Walking };

  struct PresetPosition {
    String name;
    int16_t legsPositions[8];
  };

  // Structure to hold IK calculation results
  struct IKResult {
    bool success;
    float theta1;  // First joint angle in degrees
    float theta2;  // Second joint angle in degrees
  };

  Robot();
  void init();
  void update();
  void moveLegs(int16_t* positions, uint16_t duration);

  String getJson();
  String getCurrentPositionName();
  void applyPresetPosition(const String& name);
  String* getAvailablePositionNames();
 
  // Inverse kinematics calculation
  IKResult calculateIK(float x, float y);
  
  // Set robot global height using IK
  void setRobotPose(float height, float pitch, float roll, uint16_t duration);

  // Walking sequence
  void walk(int stepLength, int stepHeight, int stepDuration, int walkHeight = 50, float xDirection = 0.0f, float yDirection = 0.0f, float xOffset = 0.0f);
  void stopWalking();
  
  // Set all walk parameters at once
  void setWalkParameters(int stepLength, int stepHeight, int stepDuration, int walkHeight);

  ServoGroup legs;

 private:
  State state;

  static const int numPresetPositions = 3;
  PresetPosition presetPositions[numPresetPositions] = {
      {"init", {900, 900, 900, 900, 900, 900, 900, 900}},
      {"position1", {475, 475, 475, 475, 1800, 1800, 1800, 1800}},
      {"position2", {800, 800, 800, 800, 800, 800, 800, 800}}};
  String currentPositionName = "init";

  static constexpr float segment1Length = 37.0;
  static constexpr float segment2Length = 45.0;
  static constexpr float baseWidth = 45.0;  // Distance between legs in mm
  static constexpr float baseLength = 89.0; // Distance between front and back legs in mm

  // Leg position definitions
  enum LegPosition {
    FRONT_LEFT = 0,
    FRONT_RIGHT = 1,
    BACK_LEFT = 2,
    BACK_RIGHT = 3
  };

  // Default walking parameters
  int defaultStepLength = 70;
  int defaultStepHeight = 10;
  int defaultStepDuration = 400;
  int defaultWalkHeight = 65;

  // Walking state variables
  bool isWalking = false;
  int currentPhase = 0;
  unsigned long phaseStartTime = 0;
  int walkStepLength = 0;
  int walkStepHeight = 0;
  int walkStepDuration = 0;
  
  int walkHeight = 50;  // Default walk height (percentage of maxHeight)
  float walkXDirection = 0.0f;  // X direction from -1.0 to 1.0
  float walkYDirection = 0.0f;  // Y direction from -1.0 to 1.0
  float walkXOffset = 0.0f;     // X offset for leg positioning

  // Walking sequence update
  void updateWalking();
};

#endif  // Robot_H