# common

Shared protocol header included by all three nodes (`pc`, `pi`, `pico`). No external dependencies — pure C++17.

## File

| File | Description |
|------|-------------|
| `protocol.hpp` | `ControlPacket` + `FeedbackPacket` structs, constants, and CRC helpers |

## ControlPacket Layout (16 bytes)

| Byte(s) | Field      | Type     | Range          | Description |
|---------|------------|----------|----------------|-------------|
| 0       | magic      | `uint8_t` | `0xAB`        | Start-of-frame marker |
| 1       | seq        | `uint8_t` | 0–255, wraps  | Sequence counter (drop detection) |
| 2–5     | throttle   | `float`   | [-1.0, 1.0]   | Forward (+) / backward (−) speed |
| 6–9     | steer      | `float`   | [-1.0, 1.0]   | Right (+) / left (−) steering |
| 10–13   | aux        | `float`   | 0.0           | Reserved — send 0.0 |
| 14      | actions    | `uint8_t` | bitmask       | Discrete action flags (see below) |
| 15      | crc        | `uint8_t` | —             | XOR of bytes 0–14 |

## Action Bitmask (byte 14)

| Bit | Name          | Effect |
|-----|---------------|--------|
| 0   | `TOGGLE_PIVOT` | Switch between Ackermann and pivot (spin-in-place) mode |
| 1   | `BOOST`        | High-speed mode |
| 2   | `BRAKE`        | Active braking |

## FeedbackPacket Layout (35 bytes)

Sent by the Pico every 20 ms and relayed by the Pi to the PC on UDP port `5006`.

| Byte(s) | Field             | Type      | Range           | Description |
|---------|-------------------|-----------|-----------------|-------------|
| 0       | magic             | `uint8_t` | `0xCD`          | Start-of-frame marker |
| 1       | seq               | `uint8_t` | 0–255, wraps    | Sequence counter |
| 2–17    | steer_angles[4]   | `float[4]`| [0°, 180°]      | Measured steering angles — FL, FR, RL, RR |
| 18–33   | traction_duty[4]  | `float[4]`| [−100, 100]     | Applied PWM duty per wheel — FL, FR, RL, RR |
| 34      | crc               | `uint8_t` | —               | XOR of bytes 0–33 |

`traction_duty` reflects the last value written to the H-bridge. There are no tachometers; actual wheel speed is not measured.

## CRC Helpers

All six functions are `inline` in `protocol.hpp` — overloaded for each packet type:

| Function | Description |
|----------|-------------|
| `computeCrc(pkt)` | Returns XOR of all bytes except the last |
| `stampCrc(pkt)` | Writes the computed CRC into the last byte |
| `validateCrc(pkt)` | Returns `true` if magic and CRC are both valid |

Works identically for `ControlPacket` and `FeedbackPacket`.
