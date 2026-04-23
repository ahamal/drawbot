#pragma once
#include <Arduino.h>

enum class Op : uint8_t {
  None,
  G0,    // rapid move (caller handles pen-up convention)
  G1,    // linear move
  G28,   // home
  G90,   // absolute coordinates
  G91,   // relative coordinates
  M3,    // pen down
  M5,    // pen up
};

struct Command {
  Op op = Op::None;
  float x = 0, y = 0, f = 0;
  bool has_x = false, has_y = false, has_f = false;
};

// Parse one line. Returns false on syntax error. An empty/comment line is
// success with op == Op::None — caller skips those.
bool parseLine(const String& line, Command& out);
