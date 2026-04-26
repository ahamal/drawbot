#pragma once
#include <Arduino.h>
#include <vector>
#include "gcode.h"

enum class MotionState : uint8_t { Idle, Moving, PenSettling, Homing, Error };
enum class PenState   : uint8_t { Up, Down };

struct Status {
  MotionState state;
  PenState    pen;
  float       x_mm, y_mm;
  size_t      queue_remaining;
  String      last_error;
};

struct MotionConfig {
  float max_speed_sps;     // top speed for any single axis
  float acceleration_sps2; // ramp rate
};

void motion_init();
void motion_tick();   // call from loop()

// Returns false (and sets `err`) if a job is already running.
bool   motion_enqueue(const std::vector<Command>& cmds, String& err);
void   motion_abort();
Status motion_status();

// Runtime-tunable speed/accel (UI exposes these via /config).
// Persisted to NVS so changes survive reboot. Defaults come from config.h.
void         motion_load_config();                                // call before motion_init()
void         motion_set_config(float max_speed, float acceleration);
MotionConfig motion_get_config();
