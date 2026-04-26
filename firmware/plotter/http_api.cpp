#include "http_api.h"
#include "motion.h"
#include "gcode.h"
#include "web_assets.h"
#include <WebServer.h>

static WebServer server(80);

static const char* stateName(MotionState s) {
  switch (s) {
    case MotionState::Idle:        return "idle";
    case MotionState::Moving:      return "moving";
    case MotionState::Homing:      return "homing";
    case MotionState::PenSettling: return "pen_settling";
    case MotionState::Error:       return "error";
  }
  return "unknown";
}

static void handleStatus() {
  Status s = motion_status();
  String body = "{";
  body += "\"state\":\"";          body += stateName(s.state); body += "\",";
  body += "\"pen\":\"";            body += (s.pen == PenState::Down ? "down" : "up"); body += "\",";
  body += "\"x_mm\":";             body += String(s.x_mm, 2); body += ",";
  body += "\"y_mm\":";             body += String(s.y_mm, 2); body += ",";
  body += "\"queue_remaining\":";  body += String((unsigned)s.queue_remaining); body += ",";
  body += "\"last_error\":";
  if (s.last_error.length() == 0) body += "null";
  else { body += "\""; body += s.last_error; body += "\""; }
  body += "}";
  server.send(200, "application/json", body);
}

static void handleJob() {
  if (!server.hasArg("plain")) {
    server.send(400, "text/plain", "expected text/plain G-code body\n");
    return;
  }
  String src = server.arg("plain");
  std::vector<Command> cmds;
  cmds.reserve(64);

  int line_start = 0;
  int line_no = 0;
  while (line_start <= (int)src.length()) {
    int nl = src.indexOf('\n', line_start);
    String line = (nl < 0) ? src.substring(line_start) : src.substring(line_start, nl);
    line_no++;
    Command c;
    if (!parseLine(line, c)) {
      String err = "parse error on line " + String(line_no) + ": " + line;
      server.send(400, "text/plain", err);
      return;
    }
    if (c.op != Op::None) cmds.push_back(c);
    if (nl < 0) break;
    line_start = nl + 1;
  }

  String err;
  if (!motion_enqueue(cmds, err)) {
    server.send(409, "text/plain", err + "\n");
    return;
  }
  String body = "{\"queued\":" + String((unsigned)cmds.size()) + "}";
  server.send(202, "application/json", body);
}

static void handleStop() {
  motion_abort();
  server.send(200, "text/plain", "stopped\n");
}

static void handleHome() {
  std::vector<Command> cmds;
  Command c; c.op = Op::G28;
  cmds.push_back(c);
  String err;
  if (!motion_enqueue(cmds, err)) {
    server.send(409, "text/plain", err + "\n");
    return;
  }
  server.send(202, "text/plain", "homing\n");
}

static String configJson() {
  MotionConfig c = motion_get_config();
  return String("{\"max_speed\":") + String(c.max_speed_sps, 0) +
         ",\"acceleration\":" + String(c.acceleration_sps2, 0) + "}";
}

static void handleGetConfig() {
  server.send(200, "application/json", configJson());
}

static void handleSetConfig() {
  // Form-encoded body: max_speed=<sps>&acceleration=<sps2>. Either may be omitted.
  MotionConfig cur = motion_get_config();
  float max_speed = cur.max_speed_sps;
  float accel     = cur.acceleration_sps2;
  if (server.hasArg("max_speed"))    max_speed = server.arg("max_speed").toFloat();
  if (server.hasArg("acceleration")) accel     = server.arg("acceleration").toFloat();
  motion_set_config(max_speed, accel);
  server.send(200, "application/json", configJson());
}

static void serveStatic(const char* contentType, const char* body) {
  server.sendHeader("Cache-Control", "no-cache");
  server.send_P(200, contentType, body);
}

void http_api_init() {
  // SPA — vite-plugin-singlefile inlines all JS/CSS/assets into one HTML.
  server.on("/", HTTP_GET, []() { serveStatic("text/html", web_assets::INDEX_HTML); });

  // JSON API.
  server.on("/status", HTTP_GET,  handleStatus);
  server.on("/job",    HTTP_POST, handleJob);
  server.on("/stop",   HTTP_POST, handleStop);
  server.on("/home",   HTTP_POST, handleHome);
  server.on("/config", HTTP_GET,  handleGetConfig);
  server.on("/config", HTTP_POST, handleSetConfig);

  server.onNotFound([]() {
    server.send(404, "text/plain", "TinyRoboCo: try /, /status, /job, /stop, /home, /config\n");
  });
  server.begin();
}

void http_api_tick() { server.handleClient(); }
