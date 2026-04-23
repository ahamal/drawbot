# firmware/

Firmware ladder. Each stage validates one thing and is meant to be flashed in order.

| Stage | Folder | What it proves |
|---|---|---|
| 1 | `bringup/stepper_test/` | One stepper spins under AccelStepper control |
| 2 | `bringup/xy_test/` | Two steppers + MultiStepper coordination |
| 3 | `bringup/servo_test/` | Servo sweeps cleanly without browning out the rail |
| 4 | `plotter/` | All hardware unified, G-code over Wi-Fi REST |

The bring-up sketches stay in the repo as reference. They're the way you debug: if the main `plotter/` firmware misbehaves, drop back to the relevant bring-up to isolate hardware vs. software.
