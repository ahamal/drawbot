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

void motion_init();
void motion_tick();   // call from loop()

// Returns false (and sets `err`) if a job is already running.
bool   motion_enqueue(const std::vector<Command>& cmds, String& err);
void   motion_abort();
Status motion_status();
