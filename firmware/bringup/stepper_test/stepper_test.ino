// TinyRoboCo — stepper bring-up
// 28BYJ-48 + ULN2003 driver, ESP32, half-step mode via AccelStepper.
// Spins one full output-shaft revolution forward, then back, forever.
//
// Wiring (Y motor):
//   ULN2003 IN1 -> GPIO 18
//   ULN2003 IN2 -> GPIO 19
//   ULN2003 IN3 -> GPIO 21
//   ULN2003 IN4 -> GPIO 22
//   ULN2003 +   -> CanaKit 5V
//   ULN2003 -   -> CanaKit GND  (and also tied to ESP32 GND)
//   ESP32 powered via USB from Mac.

#include <AccelStepper.h>

#define IN1 18
#define IN2 19
#define IN3 21
#define IN4 22

// Pin order (IN1, IN3, IN2, IN4) is the coil-pair ordering AccelStepper expects
// when driving a 28BYJ-48 through a ULN2003 in HALF4WIRE mode. If the motor
// vibrates or steps unevenly, this is the first thing to swap.
AccelStepper stepper(AccelStepper::HALF4WIRE, IN1, IN3, IN2, IN4);

const long STEPS_PER_REV = 4096;  // half-step, 28BYJ-48 with internal 1:64 gearbox

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println();
  Serial.println("TinyRoboCo stepper bring-up");
  Serial.println("Forward 1 rev, pause, reverse 1 rev, repeat.");

  stepper.setMaxSpeed(600);      // steps/sec
  stepper.setAcceleration(300);  // steps/sec^2
  stepper.setCurrentPosition(0);
}

void loop() {
  Serial.println("-> forward");
  stepper.runToNewPosition(STEPS_PER_REV);
  delay(500);

  Serial.println("<- reverse");
  stepper.runToNewPosition(0);
  delay(500);
}
