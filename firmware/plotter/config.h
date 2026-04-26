#pragma once

// =============================================================================
// TinyRoboCo plotter — pin assignments, hardware constants, network config.
// =============================================================================

// Y stepper (28BYJ-48 + ULN2003 #1)
#define Y_IN1 18
#define Y_IN2 19
#define Y_IN3 21
#define Y_IN4 22

// X stepper (28BYJ-48 + ULN2003 #2)
#define X_IN1 25
#define X_IN2 26
#define X_IN3 27
#define X_IN4 14

// Pen-lift servo
#define SERVO_PIN     13
#define PEN_UP_DEG    90   // direction reversed + swing halved (was 60..100, 40°)
#define PEN_DOWN_DEG  70   // now centered at 80° with a 20° swing
#define PEN_SETTLE_MS 200  // wait after write() before next command

// Kinematics — pick exactly one. The chosen one selects the X/Y → motor mapping
// in motion.cpp. Cartesian: each motor drives one axis directly. CoreXY: both
// motors are stationary, belts/threads cross, and a step combo drives both axes
// (motor A = X+Y, motor B = X−Y in step space).
#define GEOMETRY_COREXY
// #define GEOMETRY_CARTESIAN

// Workspace orientation — flip these after wiring if a jog goes the wrong way.
// They operate on (x, y) BEFORE kinematics, so the same flag has the same
// operator-visible meaning regardless of geometry.
//
// Typical calibration: jog +X 10 from the UI. Right? Else flip INVERT_X.
//                      jog +Y 10. Down? Else flip INVERT_Y.
//                      Did +X move along Y instead? Flip SWAP_XY.
#define INVERT_X 0
#define INVERT_Y 0
#define SWAP_XY  0

// Motion
#define STEPS_PER_REV 4096                                  // half-step, 1:64 gearbox
#define DRUM_DIAM_MM  10.0f
#define STEPS_PER_MM  (STEPS_PER_REV / (3.14159265f * DRUM_DIAM_MM))
#define MAX_SPEED_SPS 1000.0f
#define ACCEL_SPS2    500.0f

// Build envelope (mm). Soft limits — commands are clamped to these.
// Bumped to 3000 for bench testing (no frame yet). Tighten back to A5 once
// the carriage is built: 148 (X) × 210 (Y).
#define X_MAX_MM 3000.0f
#define Y_MAX_MM 3000.0f

// Network
#define HOSTNAME           "drawbot"          // -> http://drawbot.local
#define SETUP_AP_NAME      "TinyRoboCo-Setup" // captive portal SSID on first boot
#define SETUP_AP_TIMEOUT_S 180                // bail out of AP mode after 3 min idle
