#!/usr/bin/env bash
# End-to-end build + flash for drawbot.
#
#   ./flash.sh                  # auto-detect ESP32 port, build everything, upload
#   PORT=/dev/cu.foo ./flash.sh # override port
#
# Requires: node, arduino-cli (with esp32 core + AccelStepper + ESP32Servo libs).
set -euo pipefail

REPO_ROOT="$(cd "$(dirname "$0")" && pwd)"
cd "$REPO_ROOT"

PORT="${PORT:-$(ls /dev/cu.usbserial-* 2>/dev/null | head -1)}"
if [ -z "$PORT" ]; then
  echo "ERROR: no /dev/cu.usbserial-* port found. Set PORT=... to override." >&2
  exit 1
fi
echo "Port: $PORT"

if [ ! -d web/node_modules ]; then
  echo "==> Installing web deps"
  ( cd web && npm install )
fi

echo "==> Building SPA"
( cd web && npm run build )

echo "==> Baking dist/index.html into firmware/plotter/web_assets.h"
./scripts/build_web.sh

echo "==> Compiling firmware"
arduino-cli compile --fqbn esp32:esp32:esp32 \
  --build-path /tmp/drawbot-build firmware/plotter

echo "==> Flashing"
arduino-cli upload --fqbn esp32:esp32:esp32 -p "$PORT" \
  --input-dir /tmp/drawbot-build firmware/plotter

echo "==> Done."
