# pico — RP2040 Firmware

Arduino-framework firmware for the Raspberry Pi Pico. Receives `ControlPacket` frames over USB-CDC serial, runs a 50 Hz control loop, drives 4 traction motors + 4 steering motors with closed-loop PID angle control, and sends a `FeedbackPacket` (steering angles + traction PWM duty) back over serial after each cycle.

## Build & Flash

```sh
pio run --target upload
```

Requires PlatformIO with the `raspberrypi` platform and `arduino` framework installed.

## Hardware Pinout

### Traction motors (H-bridge pin pairs)

| Wheel | Pin + | Pin − |
|-------|-------|-------|
| Front-left (FL) | GPIO 0 | GPIO 1 |
| Front-right (FR) | GPIO 2 | GPIO 3 |
| Rear-left (RL) | GPIO 4 | GPIO 5 |
| Rear-right (RR) | GPIO 6 | GPIO 7 |

### Steering motors (H-bridge pin pairs)

| Wheel | Pin + | Pin − |
|-------|-------|-------|
| FL | GPIO 8 | GPIO 9 |
| RL | GPIO 10 | GPIO 11 |
| FR | GPIO 12 | GPIO 13 |
| RR | GPIO 14 | GPIO 15 |

### Encoders (I2C)

| Signal | GPIO |
|--------|------|
| SDA (Wire1) | GPIO 26 |
| SCL (Wire1) | GPIO 27 |

- **TCA9548A** I2C mux at address `0x70` — one channel per wheel
- **AS5600** magnetic angle sensors at address `0x36` per channel
- I2C clock: 100 kHz

## Drive Modes

| Mode | Activation | Behavior |
|------|-----------|----------|
| **Ackermann** | Default | Normal Ackermann steering geometry; inner/outer wheel angles computed from `steer` input |
| **Pivot** | `TOGGLE_PIVOT` button | Wheels set to X pattern (36°/144°), diagonal pairs spin in opposite directions for spin-in-place |

Mode toggles on the rising edge of `TOGGLE_PIVOT`.

## Key Constants

| Constant | Value | Description |
|----------|-------|-------------|
| `L_HALF` | 0.35 m | Half wheelbase |
| `W_HALF` | 0.25 m | Half track width |
| `R_MIN` | 0.6 m | Minimum turning radius at full steer deflection |
| `Kp` | 3.5 | PID proportional gain |
| `Ki` | 12.0 | PID integral gain |
| `Kd` | 1.0 | PID derivative gain |
| PWM freq | 1 kHz | `analogWriteFreq(1000)` |
| PWM range | 16-bit (65535) | `analogWriteRange(65535)` |

## PID Steering Servo

Each steering wheel runs an independent PID loop at 50 Hz:

- **Dead zone:** ±1° — stops output and resets integral when within target
- **Anti-windup:** integral clamped to ±20
- **Stall prevention:** minimum output magnitude ramped from 25→10 as error approaches 5°
- Angle feedback from AS5600 encoders (0.1° resolution after calibration)

## Telemetry Feedback

After every control loop iteration, the firmware sends a 35-byte `FeedbackPacket` (magic `0xCD`) over USB-CDC serial:

| Field | Content |
|-------|---------|
| `steer_angles[4]` | Measured wheel steering angles [°] — FL, FR, RL, RR |
| `traction_duty[4]` | Last applied traction PWM duty [−100, 100] — FL, FR, RL, RR |

The Pi bridge reads these frames and relays them to the PC on UDP port `5006`. `traction_duty` reflects the H-bridge command, not a measured speed — there are no tachometers.
