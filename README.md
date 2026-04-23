# drawbot

DIY cardboard + skewer pen plotter. ESP32 firmware + Svelte web UI.

## What it is

~A5 cartesian pen plotter built from cardboard, bamboo skewers, thread drives, and hobby-grade 28BYJ-48 stepper motors with an SG90 servo for the pen lift. Designed to be built from junk-bin parts before any upgrade path.

## Status

- ✅ Hardware bring-up: stepper, XY coordination, servo
- ✅ Firmware: G-code parser, motion state machine, HTTP REST API, Wi-Fi
- ✅ Web UI: Svelte SPA baked into firmware, served at `/`
- ⏳ Physical assembly + calibration
- ⏳ Photo → trace → G-code pipeline (UI side)
- ⏳ Tutorial prose

## Hardware

- ESP32 dev board
- 2× 28BYJ-48 stepper + ULN2003 driver board
- 1× SG90 micro servo
- 5V 2.5A PSU (CanaKit or similar)
- Corrugated cardboard, bamboo skewers, plastic straws, polyester thread, rubber bands

## Quick start

Wire per the pin map in `firmware/plotter/config.h`. Then:

```bash
( cd web && npm install && npm run build )
./scripts/build_web.sh
arduino-cli compile --fqbn esp32:esp32:esp32 firmware/plotter
arduino-cli upload  --fqbn esp32:esp32:esp32 -p /dev/cu.usbserial-XXX firmware/plotter
```

On first boot the ESP32 hosts an open Wi-Fi AP `TinyRoboCo-Setup`. Connect a phone, enter your home network's credentials in the captive portal, save. The device then comes up at `http://drawbot.local` on your LAN. (Optional: drop a `firmware/plotter/secrets.h` to skip the portal — see `secrets.example.h`.)

## Repo layout

```
firmware/   ESP32 firmware
  bringup/  one-off sketches that validate each piece of hardware in isolation
  plotter/  main firmware: G-code, motion, REST API, Wi-Fi, embedded SPA
web/        Vite + Svelte SPA (source)
scripts/    glue scripts (web → firmware)
tools/      laptop-side helpers (planned: gcode sender, svg2gcode)
tutorial/   prose chapters (planned)
```

Each subdirectory has its own `README.md` with the details specific to that layer.

## Tutorial

Coming. The codebase is structured as a learning ladder:

1. Hardware bring-up — one stepper, then two, then the servo
2. Integration firmware — G-code, async motion state machine, REST API
3. Wi-Fi + captive portal + mDNS
4. Embedded SPA with modern build pipeline (Vite + Svelte)
5. Laptop-side tooling — sender, SVG conversion
6. Physical assembly + calibration

Each step is a chapter; the source for each lives in the matching folder.

## License

TBD.
