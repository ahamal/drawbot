# servo_test

Sweeps the SG90 between two angles to simulate pen-up / pen-down.

## Wiring

| SG90 wire | Goes to |
|---|---|
| Orange (signal) | ESP32 GPIO 13 |
| Red (+V) | **CanaKit 5V rail** — not ESP32 5V |
| Brown (GND) | CanaKit GND (already bonded to ESP32 GND) |

**470µF electrolytic cap across the servo's red and brown wires, AS CLOSE TO THE SERVO AS POSSIBLE.** The SG90's stall current can briefly hit ~700mA when changing direction; without the cap nearby, the rail dips and the ESP32 may brown-out (random reset).

Polarity matters for the cap: long leg = `+` (red wire side), short leg = `−` (brown wire side). Backwards = pop.

## Flash

```bash
arduino-cli compile --fqbn esp32:esp32:esp32 firmware/bringup/servo_test
arduino-cli upload  --fqbn esp32:esp32:esp32 -p /dev/cu.usbserial-0001 firmware/bringup/servo_test
arduino-cli monitor -p /dev/cu.usbserial-0001 -c baudrate=115200
```

## What success looks like

- Serial prints `PEN DOWN` / `PEN UP` once per second.
- Servo horn snaps between the two positions on cue.
- ESP32 does **not** reset (no boot banner repeating in the serial monitor) — that would mean rail brownout, fix the cap.

## Calibration after assembly

Once the pen mount is built, edit `PEN_UP` and `PEN_DOWN`:
- **Pen up:** smallest angle that fully lifts the pen tip off the paper (~2-3 mm clearance is plenty).
- **Pen down:** angle that puts gentle, even pressure on the paper without bending the carriage. Add a small spring or rubber band so the pen "rides" rather than rams into the paper.

## If something's wrong

| Symptom | Likely cause |
|---|---|
| Servo doesn't move at all | Signal wire on wrong pin, or +V/GND not on CanaKit rail |
| Servo twitches but doesn't reach target | Pulse width range wrong — try `attach(SERVO_PIN, 1000, 2000)` (narrower range, safer for sketchy clones) |
| ESP32 keeps resetting (boot banner repeats in serial) | Brownout from servo inrush — get the 470µF closer to the servo, or add a 100µF in parallel |
| Servo buzzes constantly at one position | Hitting mechanical end-stop — narrow the angle range |
