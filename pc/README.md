# pc — PC Controller Client

Polls an Xbox-compatible gamepad at 50 Hz via SDL2 and sends `ControlPacket` UDP datagrams to the Raspberry Pi bridge. Simultaneously receives `FeedbackPacket` datagrams from the Pi and prints steering angles and traction PWM duty to stdout.

## Dependencies

| Dependency | Notes |
|------------|-------|
| SDL2 | Gamepad input |
| CMake ≥ 3.16 | Build system |
| C++17 compiler | GCC, Clang, or MSVC |
| Winsock2 | Windows only — linked automatically |

## Build

```sh
cmake -B build
cmake --build build
```

## Run

```sh
./build/hermes-pc <pi-ip> [port]
# Example:
./build/hermes-pc 192.168.1.50 5005
```

Default control port is `5005`. Feedback is received on port `5006` (fixed). Connect an Xbox controller before launching.

## Controller Mapping

| Input | SDL2 | Packet Field |
|-------|------|-------------|
| Right trigger | Axis 5 | `throttle` > 0 (forward) |
| Left trigger | Axis 2 | `throttle` < 0 (backward) |
| Left stick X | Axis 0 | `steer` |
| Button A | Button 0 | `TOGGLE_PIVOT` action |
| Left bumper (LB) | Button 4 | `BOOST` action |
| Right bumper (RB) | Button 5 | `BRAKE` action |

**Dead zones:** 8% on steer axis, 5% on triggers.

**Trigger priority:** once one trigger activates, the other is locked out until both are fully released — prevents simultaneous forward/backward commands.

## Feedback Output

When a `FeedbackPacket` is received from the Pi, the following is printed to stdout:

```
[rx] angles FL=<°> FR=<°> RL=<°> RR=<°>  duty FL=<%%> FR=<%%> RL=<%%> RR=<%%>
```

- **angles** — measured wheel steering angles in degrees (90° = straight ahead)
- **duty** — last applied traction PWM duty [−100, 100] per wheel
