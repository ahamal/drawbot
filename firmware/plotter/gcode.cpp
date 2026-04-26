#include "gcode.h"

static String stripComments(const String& line) {
  String s = line;
  int semi = s.indexOf(';');
  if (semi >= 0) s.remove(semi);
  while (true) {
    int o = s.indexOf('(');
    int c = (o >= 0) ? s.indexOf(')', o) : -1;
    if (o < 0 || c < 0) break;
    s.remove(o, c - o + 1);
  }
  s.trim();
  return s;
}

bool parseLine(const String& raw, Command& out) {
  out = Command{};
  String s = stripComments(raw);
  if (s.length() == 0) return true;

  int i = 0;
  while (i < (int)s.length()) {
    while (i < (int)s.length() && isspace(s[i])) i++;
    int start = i;
    while (i < (int)s.length() && !isspace(s[i])) i++;
    if (start == i) break;

    String tok = s.substring(start, i);
    if (tok.length() < 2) return false;
    char letter = toupper(tok[0]);
    float val = tok.substring(1).toFloat();
    int ival = (int)val;

    switch (letter) {
      case 'G':
        if      (ival == 0)  out.op = Op::G0;
        else if (ival == 1)  out.op = Op::G1;
        else if (ival == 28) out.op = Op::G28;
        else if (ival == 92) out.op = Op::G92;
        else if (ival == 90) out.op = Op::G90;
        else if (ival == 91) out.op = Op::G91;
        else return false;
        break;
      case 'M':
        if      (ival == 3) out.op = Op::M3;
        else if (ival == 5) out.op = Op::M5;
        else return false;
        break;
      case 'X': out.x = val; out.has_x = true; break;
      case 'Y': out.y = val; out.has_y = true; break;
      case 'F': out.f = val; out.has_f = true; break;
      default:  return false;
    }
  }
  return true;
}
