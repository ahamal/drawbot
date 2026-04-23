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
#define PEN_UP_DEG    60   // calibrate after pen mount built
#define PEN_DOWN_DEG  100
#define PEN_SETTLE_MS 200  // wait after write() before next command

// Motion
#define STEPS_PER_REV 4096                                  // half-step, 1:64 gearbox
#define DRUM_DIAM_MM  10.0f
#define STEPS_PER_MM  (STEPS_PER_REV / (3.14159265f * DRUM_DIAM_MM))
#define MAX_SPEED_SPS 1000.0f
#define ACCEL_SPS2    500.0f

// Build envelope (mm). Soft limits — commands are clamped to these.
#define X_MAX_MM 148.0f    // ~A5 width
#define Y_MAX_MM 210.0f    // ~A5 height

// Network
#define HOSTNAME           "drawbot"          // -> http://drawbot.local
#define SETUP_AP_NAME      "TinyRoboCo-Setup" // captive portal SSID on first boot
#define SETUP_AP_TIMEOUT_S 180                // bail out of AP mode after 3 min idle
