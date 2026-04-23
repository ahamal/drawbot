# xy_test

Brings up the second stepper and validates two motors driven from one ESP32 + one PSU.

Cycle each iteration:

1. **Y solo** — one rev forward, pause, reverse.
2. **X solo** — one rev forward, pause, reverse.
3. **Both independent** — both targeted simultaneously; whichever finishes first stops, the other keeps going. Useful for checking that the two drivers don't electrically interfere.
4. **Coordinated diagonal** — `MultiStepper` paces both axes to finish together (Y moves a full rev, X moves a half rev — should arrive at the same instant).

## Flash

```bash
arduino-cli compile --fqbn esp32:esp32:esp32 firmware/bringup/xy_test
arduino-cli upload  --fqbn esp32:esp32:esp32 -p /dev/cu.usbserial-0001 firmware/bringup/xy_test
arduino-cli monitor -p /dev/cu.usbserial-0001 -c baudrate=115200
```

## What success looks like

- Each phase runs in order, serial labels each one.
- Both motors run smoothly with no audible buzzing during simultaneous movement.
- Coordinated diagonal: both motors stop at exactly the same instant. If X stops noticeably earlier, `MultiStepper` is doing its job (X had less distance and runs slower to match).

## If something's wrong

| Symptom | Likely cause |
|---|---|
| One motor works solo, both stalls when run together | PSU sag — check CanaKit isn't being pulled below 4.5 V; tighten power wiring or move 470µF cap closer |
| New (X) motor doesn't move at all | Pin order in constructor wrong, or one of the IN pins not landed on the right ULN2003 input |
| New motor moves but the other now misbehaves | Ground loop — make sure both ULN2003 GNDs and ESP32 GND meet at one point on the CanaKit GND rail |
| Coordinated diagonal looks staircase-y | Expected — `MultiStepper` runs at constant speed without acceleration, so quick start/stop. Will smooth out under real G-code planner. |
