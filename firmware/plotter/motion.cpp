#include "motion.h"
#include "config.h"
#include <AccelStepper.h>
#include <MultiStepper.h>
#include <ESP32Servo.h>

// Coil-pair ordering (IN1, IN3, IN2, IN4) — see bringup/stepper_test for why.
static AccelStepper yStepper(AccelStepper::HALF4WIRE, Y_IN1, Y_IN3, Y_IN2, Y_IN4);
static AccelStepper xStepper(AccelStepper::HALF4WIRE, X_IN1, X_IN3, X_IN2, X_IN4);
static MultiStepper xyGroup;
static Servo        penServo;

static MotionState gState     = MotionState::Idle;
static PenState    gPen       = PenState::Up;
static bool        gAbsolute  = true;
static float       gX_mm      = 0;
static float       gY_mm      = 0;
static String      gLastError;

static std::vector<Command> gQueue;
static size_t        gQueuePos = 0;
static unsigned long gPenSettleUntil = 0;

static long mmToSteps(float mm) { return (long)lroundf(mm * STEPS_PER_MM); }

static void clamp(float& x, float lo, float hi) {
  if (x < lo) x = lo; if (x > hi) x = hi;
}

static void startMoveTo(float x, float y) {
  clamp(x, 0, X_MAX_MM);
  clamp(y, 0, Y_MAX_MM);
  // Order matches xyGroup.addStepper() order below: y then x.
  long target[2] = { mmToSteps(y), mmToSteps(x) };
  xyGroup.moveTo(target);
  gX_mm = x; gY_mm = y;
  gState = MotionState::Moving;
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
  long target[2] = { 0, 0 };
  xyGroup.moveTo(target);
  gX_mm = 0; gY_mm = 0;
  gState = MotionState::Homing;
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
    case Op::G90: gAbsolute = true;  gState = MotionState::Idle; break;
    case Op::G91: gAbsolute = false; gState = MotionState::Idle; break;
    case Op::M3:  setPen(PenState::Down); break;
    case Op::M5:  setPen(PenState::Up);   break;
    case Op::None: gState = MotionState::Idle; break;
  }
}

void motion_init() {
  xStepper.setMaxSpeed(MAX_SPEED_SPS);
  xStepper.setAcceleration(ACCEL_SPS2);
  yStepper.setMaxSpeed(MAX_SPEED_SPS);
  yStepper.setAcceleration(ACCEL_SPS2);
  xStepper.setCurrentPosition(0);
  yStepper.setCurrentPosition(0);
  xyGroup.addStepper(yStepper);   // index 0
  xyGroup.addStepper(xStepper);   // index 1

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
      if (!xyGroup.run()) gState = MotionState::Idle;
      break;
    case MotionState::PenSettling:
      if ((long)(millis() - gPenSettleUntil) >= 0) gState = MotionState::Idle;
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
  // Hard-stop: target = current position, motors halt on next tick.
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
