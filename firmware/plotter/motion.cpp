#include "motion.h"
#include "config.h"
#include <AccelStepper.h>
#include <ESP32Servo.h>
#include <Preferences.h>

// In Cartesian mode, these are literally the X and Y motors. In CoreXY mode,
// they are motor A (using the "Y" pin block) and motor B (using the "X" pin
// block) — names kept for code reuse and for the bringup sketches.
static AccelStepper yStepper(AccelStepper::HALF4WIRE, Y_IN1, Y_IN3, Y_IN2, Y_IN4);
static AccelStepper xStepper(AccelStepper::HALF4WIRE, X_IN1, X_IN3, X_IN2, X_IN4);
static Servo        penServo;
static Preferences  prefs;

static MotionState gState     = MotionState::Idle;
static PenState    gPen       = PenState::Up;
static bool        gAbsolute  = true;
static float       gX_mm      = 0;
static float       gY_mm      = 0;
static String      gLastError;

// Runtime config — loaded from NVS at boot, defaults from config.h.
static float gMaxSpeed     = MAX_SPEED_SPS;
static float gAcceleration = ACCEL_SPS2;

static std::vector<Command> gQueue;
static size_t        gQueuePos = 0;
static unsigned long gPenSettleUntil = 0;

static long mmToSteps(float mm) { return (long)lroundf(mm * STEPS_PER_MM); }

static void clamp(float& x, float lo, float hi) {
  if (x < lo) x = lo; if (x > hi) x = hi;
}

// Map workspace (x, y) in mm to motor step targets [a_steps, b_steps].
// Indices match constructor order: out[0] = yStepper, out[1] = xStepper.
static void mapXYtoSteps(float x, float y, long out[2]) {
  // Workspace-level corrections — applied before kinematics so they read the
  // same way whether you're using Cartesian or CoreXY.
#if SWAP_XY
  { float t = x; x = y; y = t; }
#endif
#if INVERT_X
  x = -x;
#endif
#if INVERT_Y
  y = -y;
#endif

#if defined(GEOMETRY_COREXY)
  // CoreXY: motor A = X+Y, motor B = X−Y (in step space).
  out[0] = mmToSteps(x + y);
  out[1] = mmToSteps(x - y);
#elif defined(GEOMETRY_CARTESIAN)
  out[0] = mmToSteps(y);
  out[1] = mmToSteps(x);
#else
  #error "Pick a geometry in config.h: GEOMETRY_COREXY or GEOMETRY_CARTESIAN"
#endif
}

// Per-move XY coordination: scale each axis's max speed AND acceleration by
// its share of the larger axis's distance. Both axes then trace the same
// trapezoidal velocity profile shape, just scaled, so they finish together
// and the diagonal stays straight.
static void startMoveTo(float x, float y) {
  // Symmetric soft envelope: -MAX..+MAX. Allows jogging negative without
  // endstops — origin is wherever G92 last set it (or boot position).
  clamp(x, -X_MAX_MM, X_MAX_MM);
  clamp(y, -Y_MAX_MM, Y_MAX_MM);

  long target[2];
  mapXYtoSteps(x, y, target);

  long dy = labs(target[0] - yStepper.currentPosition());
  long dx = labs(target[1] - xStepper.currentPosition());
  long dmax = (dx > dy) ? dx : dy;

  if (dmax == 0) {
    gX_mm = x; gY_mm = y;
    gState = MotionState::Idle;
    return;
  }

  float yRatio = (float)dy / (float)dmax;
  float xRatio = (float)dx / (float)dmax;
  yStepper.setMaxSpeed(gMaxSpeed     * yRatio);
  yStepper.setAcceleration(gAcceleration * yRatio);
  xStepper.setMaxSpeed(gMaxSpeed     * xRatio);
  xStepper.setAcceleration(gAcceleration * xRatio);

  yStepper.moveTo(target[0]);
  xStepper.moveTo(target[1]);

  gX_mm = x; gY_mm = y;
  gState = MotionState::Moving;
}

static void setPosition(float x, float y) {
  // G92: redefine current position without moving the motors. Same XY → steps
  // transform as motion, just applied directly to each stepper's counter.
  long s[2];
  mapXYtoSteps(x, y, s);
  yStepper.setCurrentPosition(s[0]);
  xStepper.setCurrentPosition(s[1]);
  gX_mm = x; gY_mm = y;
  gState = MotionState::Idle;
}

static void setPen(PenState p) {
  penServo.write(p == PenState::Down ? PEN_DOWN_DEG : PEN_UP_DEG);
  gPen = p;
  gPenSettleUntil = millis() + PEN_SETTLE_MS;
  gState = MotionState::PenSettling;
}

static void runHome() {
  // Naive home: drive to (0,0). v1 trusts the user to power-on with the
  // carriage at home. A future chapter adds limit switches.
  startMoveTo(0, 0);
  if (gState == MotionState::Moving) gState = MotionState::Homing;
}

static void executeNext() {
  if (gQueuePos >= gQueue.size()) {
    gQueue.clear();
    gQueuePos = 0;
    gState = MotionState::Idle;
    return;
  }
  const Command& c = gQueue[gQueuePos++];
  switch (c.op) {
    case Op::G0:
    case Op::G1: {
      float tx = gAbsolute ? (c.has_x ? c.x : gX_mm) : (gX_mm + (c.has_x ? c.x : 0));
      float ty = gAbsolute ? (c.has_y ? c.y : gY_mm) : (gY_mm + (c.has_y ? c.y : 0));
      startMoveTo(tx, ty);
    } break;
    case Op::G28: runHome(); break;
    case Op::G92: {
      float nx = c.has_x ? c.x : gX_mm;
      float ny = c.has_y ? c.y : gY_mm;
      setPosition(nx, ny);
    } break;
    case Op::G90: gAbsolute = true;  gState = MotionState::Idle; break;
    case Op::G91: gAbsolute = false; gState = MotionState::Idle; break;
    case Op::M3:  setPen(PenState::Down); break;
    case Op::M5:  setPen(PenState::Up);   break;
    case Op::None: gState = MotionState::Idle; break;
  }
}

void motion_load_config() {
  prefs.begin("drawbot", true);   // read-only
  gMaxSpeed     = prefs.getFloat("max_speed", MAX_SPEED_SPS);
  gAcceleration = prefs.getFloat("accel",     ACCEL_SPS2);
  prefs.end();
}

static void saveConfig() {
  prefs.begin("drawbot", false);  // read-write
  prefs.putFloat("max_speed", gMaxSpeed);
  prefs.putFloat("accel",     gAcceleration);
  prefs.end();
}

void motion_init() {
  // Set initial values; per-move scaling overrides these in startMoveTo.
  xStepper.setMaxSpeed(gMaxSpeed);
  xStepper.setAcceleration(gAcceleration);
  yStepper.setMaxSpeed(gMaxSpeed);
  yStepper.setAcceleration(gAcceleration);
  xStepper.setCurrentPosition(0);
  yStepper.setCurrentPosition(0);

  penServo.setPeriodHertz(50);
  penServo.attach(SERVO_PIN, 500, 2400);
  penServo.write(PEN_UP_DEG);
  gPen = PenState::Up;
}

void motion_tick() {
  switch (gState) {
    case MotionState::Idle:
      if (gQueuePos < gQueue.size()) executeNext();
      break;
    case MotionState::Moving:
    case MotionState::Homing:
      // Tick both motors. AccelStepper.run() is non-blocking and idempotent
      // when the motor is at its target.
      xStepper.run();
      yStepper.run();
      if (xStepper.distanceToGo() == 0 && yStepper.distanceToGo() == 0) {
        // No Idle gap — chain directly to the next command for back-to-back motion.
        if (gQueuePos < gQueue.size()) executeNext();
        else gState = MotionState::Idle;
      }
      break;
    case MotionState::PenSettling:
      if ((long)(millis() - gPenSettleUntil) >= 0) {
        if (gQueuePos < gQueue.size()) executeNext();
        else gState = MotionState::Idle;
      }
      break;
    case MotionState::Error:
      break;
  }
}

bool motion_enqueue(const std::vector<Command>& cmds, String& err) {
  if (gState != MotionState::Idle || gQueuePos < gQueue.size()) {
    err = "busy";
    return false;
  }
  gQueue = cmds;
  gQueuePos = 0;
  gLastError = "";
  return true;
}

void motion_abort() {
  gQueue.clear();
  gQueuePos = 0;
  // Hard-stop: target = current position; motors halt on next tick.
  xStepper.moveTo(xStepper.currentPosition());
  yStepper.moveTo(yStepper.currentPosition());
  penServo.write(PEN_UP_DEG);
  gPen = PenState::Up;
  gPenSettleUntil = millis() + PEN_SETTLE_MS;
  gState = MotionState::PenSettling;
  gLastError = "aborted";
}

Status motion_status() {
  return Status{
    gState, gPen, gX_mm, gY_mm,
    (gQueue.size() > gQueuePos) ? (gQueue.size() - gQueuePos) : 0,
    gLastError,
  };
}

void motion_set_config(float max_speed, float acceleration) {
  if (max_speed     > 0) gMaxSpeed     = max_speed;
  if (acceleration  > 0) gAcceleration = acceleration;
  saveConfig();
  // New values take effect on the NEXT startMoveTo. Don't disturb a move in flight.
}

MotionConfig motion_get_config() {
  return MotionConfig{ gMaxSpeed, gAcceleration };
}
