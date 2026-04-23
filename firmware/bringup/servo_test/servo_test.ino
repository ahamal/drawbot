// TinyRoboCo — servo bring-up
// SG90 micro servo on ESP32 GPIO 13. Sweeps between two angles to simulate
// pen-up / pen-down on the plotter. Timing matches what the G-code M3/M5
// pen sequence will use later.
//
// Wiring:
//   SG90 signal (orange) -> GPIO 13
//   SG90 +V    (red)     -> CanaKit 5V (NOT ESP32 5V — servo stall current
//                          can spike past 600 mA and brown out USB power)
//   SG90 GND   (brown)   -> CanaKit GND (already bonded to ESP32 GND)
//   470µF electrolytic across SG90's +V and GND, AS CLOSE TO THE SERVO AS
//   POSSIBLE — buffers the inrush current spike during direction reversals.

#include <ESP32Servo.h>

#define SERVO_PIN 13

// Calibrate later for your actual pen mount. SG90s vary; safe starting range
// is 60° / 100°. We'll dial in pen-up clearance and pen-down pressure once
// the carriage is built.
const int PEN_UP   = 60;
const int PEN_DOWN = 100;

Servo penServo;

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println();
  Serial.println("TinyRoboCo servo bring-up");
  Serial.print("Pen up = "); Serial.print(PEN_UP);
  Serial.print(" deg, Pen down = "); Serial.print(PEN_DOWN); Serial.println(" deg");

  // ESP32Servo allocates a hardware PWM channel; default 50 Hz / 500-2400 us
  // pulse width matches SG90 spec.
  penServo.setPeriodHertz(50);
  penServo.attach(SERVO_PIN, 500, 2400);

  penServo.write(PEN_UP);
  delay(500);
}

void loop() {
  Serial.println("PEN DOWN");
  penServo.write(PEN_DOWN);
  delay(1000);

  Serial.println("PEN UP");
  penServo.write(PEN_UP);
  delay(1000);
}
