// TinyRoboCo — XY stepper bring-up
// Two 28BYJ-48 + ULN2003 drivers, ESP32, half-step mode.
// Phase 1: spin Y solo (one rev fwd/back), then X solo, then both together.
// Phase 2: coordinated diagonal move via MultiStepper.
//
// Wiring:
//   Y motor (ULN2003 #1):
//     IN1 -> GPIO 18, IN2 -> GPIO 19, IN3 -> GPIO 21, IN4 -> GPIO 22
//   X motor (ULN2003 #2):
//     IN1 -> GPIO 25, IN2 -> GPIO 26, IN3 -> GPIO 27, IN4 -> GPIO 14
//   Both ULN2003s: + -> CanaKit 5V, - -> CanaKit GND (and ESP32 GND).

#include <AccelStepper.h>
#include <MultiStepper.h>

// Y motor pins
#define Y_IN1 18
#define Y_IN2 19
#define Y_IN3 21
#define Y_IN4 22

// X motor pins
#define X_IN1 25
#define X_IN2 26
#define X_IN3 27
#define X_IN4 14

// Coil-pair ordering for AccelStepper HALF4WIRE on 28BYJ-48 + ULN2003.
AccelStepper yStepper(AccelStepper::HALF4WIRE, Y_IN1, Y_IN3, Y_IN2, Y_IN4);
AccelStepper xStepper(AccelStepper::HALF4WIRE, X_IN1, X_IN3, X_IN2, X_IN4);

MultiStepper xyGroup;

const long STEPS_PER_REV = 4096;   // half-step, 28BYJ-48 with 1:64 gearbox
const float MAX_SPEED    = 1200.0; // steps/sec — push speed for bench test (no load)
const float ACCEL        = 600.0;  // steps/sec^2 — ramp matters more than top speed

void solo(AccelStepper& s, const char* label) {
  Serial.print("Solo: "); Serial.println(label);
  s.runToNewPosition(STEPS_PER_REV);
  delay(300);
  s.runToNewPosition(0);
  delay(300);
}

void both_independent() {
  Serial.println("Both, independent (one starts before the other finishes)");
  yStepper.moveTo(STEPS_PER_REV);
  xStepper.moveTo(STEPS_PER_REV);
  while (yStepper.distanceToGo() != 0 || xStepper.distanceToGo() != 0) {
    yStepper.run();
    xStepper.run();
  }
  delay(300);
  yStepper.moveTo(0);
  xStepper.moveTo(0);
  while (yStepper.distanceToGo() != 0 || xStepper.distanceToGo() != 0) {
    yStepper.run();
    xStepper.run();
  }
  delay(300);
}

void coordinated_diagonal() {
  // MultiStepper paces both motors so they finish together — required for
  // straight diagonals when steps_per_mm matches between axes.
  Serial.println("Coordinated diagonal via MultiStepper");
  long target_fwd[2] = { STEPS_PER_REV, STEPS_PER_REV / 2 };  // unequal on purpose
  long target_back[2] = { 0, 0 };
  xyGroup.moveTo(target_fwd);
  xyGroup.runSpeedToPosition();
  delay(300);
  xyGroup.moveTo(target_back);
  xyGroup.runSpeedToPosition();
  delay(300);
}

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println();
  Serial.println("TinyRoboCo XY bring-up");

  yStepper.setMaxSpeed(MAX_SPEED);
  yStepper.setAcceleration(ACCEL);
  yStepper.setCurrentPosition(0);

  xStepper.setMaxSpeed(MAX_SPEED);
  xStepper.setAcceleration(ACCEL);
  xStepper.setCurrentPosition(0);

  // MultiStepper uses constant-speed moves (no acceleration). Speeds set above
  // via setMaxSpeed are what it will use.
  xyGroup.addStepper(yStepper);
  xyGroup.addStepper(xStepper);
}

void loop() {
  solo(yStepper, "Y");
  solo(xStepper, "X");
  both_independent();
  coordinated_diagonal();
  Serial.println("--- cycle complete, restarting ---");
  delay(1000);
}
