# Albert Mini — ESP32 Quadruped Robot Dog

Albert is an 8-servo quadruped robot dog powered by an ESP32 and a PCA9685 PWM driver. He walks, spins, dances, does push-ups, and more — all controllable over Serial or Bluetooth.

![Platform](https://img.shields.io/badge/platform-ESP32-blue)
![License](https://img.shields.io/badge/license-MIT-green)
![Servos](https://img.shields.io/badge/servos-8%20DOF-orange)

---

## Hardware

| Component | Details |
|---|---|
| MCU | ESP32 (any dev board) |
| Servo Driver | PCA9685 via I²C (`0x40`) |
| Servos | 8 × standard hobby servos (legs) + 1 × neck servo (channel 15) |
| Power | 5–6 V servo supply, separate from ESP32 logic |

**Leg layout (servo channels):**

```
  FRONT
 3,2   0,1      ← FL hip/knee, FR hip/knee
 7,6   4,5      ← BL hip/knee, BR hip/knee
  BACK
```

Even channels → hips, odd channels → knees.

## Features

**Continuous gaits** — run until stopped:

- `WALK` — diagonal trot with asymmetric knee lift
- `LEFT` / `RIGHT` — spin in place

**One-shot tricks** — play once, then return to idle:

- `UP` / `DOWN` — stand / crouch
- `PUSHUPS` — 5 reps, down-and-up
- `SWING` — lateral weight shift
- `WORM` — front-back wave crawl
- `GALLOP` — fast bounding gait
- `JUMP` — crouch → explosive extend
- `DANCE` — head bob + leg shimmy
- `UNDULATE` — smooth sinusoidal body wave
- `STOP` — halt all movement

## Getting Started

### Dependencies

Install via the Arduino Library Manager or PlatformIO:

- [Adafruit PWM Servo Driver Library](https://github.com/adafruit/Adafruit-PWM-Servo-Driver-Library)
- [Wire](https://www.arduino.cc/en/Reference/Wire) (built-in)
- [BluetoothSerial](https://github.com/espressif/arduino-esp32) (included with ESP32 Arduino core)

### Upload

1. Open the `.ino` file in Arduino IDE (or import into PlatformIO).
2. Select your ESP32 board and COM port.
3. Flash at 115200 baud.
4. On boot Albert stands up and announces `Albert 100% Loaded!`.

### Control

Send commands as newline-terminated strings over **Serial** (115200 baud) or **Bluetooth** (device name: `Albert_RobotDog`):

```
# Terminal / Serial Monitor
WALK
LEFT
STOP
DANCE

# Any Bluetooth serial app on your phone works too
```

Commands are case-insensitive.

## Tuning

Key constants at the top of the file:

| Constant | Default | Purpose |
|---|---|---|
| `SERVOMIN` / `SERVOMAX` | 100 / 500 | PWM pulse range for your servos |
| `CENTER` | 325 | Neutral standing pulse |
| `AMPLITUDE` | 45 | Gait swing amplitude (degrees-ish) |
| `FREQUENCY` | 0.005 | Gait speed (higher = faster) |
| `SPEED_FACTOR` | 0.5 | Smoothing factor for idle transitions |
| `MAX_STEP` | 8 | Max pulse change per tick |
| `MOVE_STEPS` | 35 | Interpolation steps for trick transitions |
| `off_set_walk[]` | see code | Per-servo offsets to bias the standing posture |

If Albert walks crooked, adjust `off_set_walk[]`. If knees don't lift enough, increase `AMPLITUDE` or the `+20` knee boost in `runGait()`.

## Architecture

```
loop()
 ├─ read Serial / BT command
 ├─ dispatch to mode or one-shot trick
 └─ if walking/spinning → runGait(ampScale[])
    else               → updateAllServos()  (smooth idle hold)

runGait()
 ├─ hips:  CENTER + offset + amp * cos(ωt + φ)
 └─ knees: CENTER + offset + amp * max(0, cos(ωt + φ))   ← half-wave lift

tricks (runDanceSequence, etc.)
 └─ set targetPos[] → moveUntilReachedAll()  (linear interp)
```

The gait engine uses a simple sinusoidal oscillator with per-servo phase offsets for a diagonal trot pattern. Knees use a half-rectified cosine so they only lift, never push through the floor.

## License

MIT — do whatever you want with it. If you build your own Albert, I'd love to see it.
