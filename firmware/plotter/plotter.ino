// TinyRoboCo plotter — main firmware
//
// Drives two 28BYJ-48 steppers (XY) and one SG90 servo (pen lift) using a
// minimal G-code subset. Accepts jobs over HTTP REST on the local network.
//
// mDNS: http://drawbot.local
//
//   GET  /status   -> JSON: state, position, pen, queue depth
//   POST /job      body: text/plain G-code (multi-line) -> 202 or 409 (busy)
//   POST /stop     -> aborts current job, lifts pen
//   POST /home     -> runs G28
//
// First boot: ESP32 hosts an open Wi-Fi AP "TinyRoboCo-Setup". Connect with
// a phone or laptop, enter your Wi-Fi credentials in the captive portal.
// Saved to NVS for future boots.

#include "config.h"
#include "wifi_setup.h"
#include "motion.h"
#include "http_api.h"

void setup() {
  Serial.begin(115200);
  delay(300);
  Serial.println();
  Serial.println("TinyRoboCo plotter booting");

  motion_load_config();   // load speed/accel from NVS (defaults from config.h)
  motion_init();
  wifi_setup_init();
  http_api_init();

  Serial.println("Ready");
}

void loop() {
  motion_tick();
  http_api_tick();
}
