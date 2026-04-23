# stepper_test

First-light sketch: drives one 28BYJ-48 through a ULN2003 in half-step mode and spins one output-shaft revolution forward, pauses, reverses, repeats.

## Prerequisites

- Arduino IDE (or PlatformIO) with **ESP32 board support** installed
  - Arduino IDE: Boards Manager → search "esp32" → install (Espressif Systems)
- **AccelStepper** library installed
  - Arduino IDE: Library Manager → search "AccelStepper" → install (by Mike McCauley)

## Board settings (Arduino IDE)

- Board: `ESP32 Dev Module` (or whatever matches your dev board)
- Upload Speed: 921600
- Port: whichever `/dev/cu.usbserial-*` or `/dev/cu.SLAB_USBtoUART` shows up after plugging in

## What success looks like

1. Serial monitor at **115200 baud** prints `TinyRoboCo stepper bring-up`.
2. The four LEDs on the ULN2003 board chase in a smooth pattern.
3. Motor output shaft completes a full 360° rotation in ~7 seconds, pauses, then reverses.

## If something's wrong

| Symptom | Likely cause |
|---|---|
| LEDs chase but motor only buzzes/vibrates | Coil pair order wrong → swap to `HALF4WIRE, IN1, IN2, IN3, IN4` (physical order) and retest |
| Motor turns the wrong direction | Cosmetic — swap any logical pin pair, or just negate the target |
| Motor stalls / loses steps | Drop `setMaxSpeed` from 600 → 400, retry |
| Nothing happens, no LEDs | ULN2003 not powered (no 5V on `+` pin) or grounds not bonded |
| Sketch won't upload | Hold `BOOT` on ESP32 while clicking upload, release after "Connecting..." appears |
