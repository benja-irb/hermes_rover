# pi — Raspberry Pi Bridge

Bidirectional relay between the PC and the Pico. Forwards `ControlPacket` UDP datagrams from the PC to the Pico over USB-CDC serial, and relays `FeedbackPacket` frames from the Pico back to the PC over UDP. Logs dropped control packets detected via sequence number gaps.

## Dependencies

| Dependency | Notes |
|------------|-------|
| CMake ≥ 3.16 | Build system |
| C++17 compiler | Standard POSIX system (no extra libs) |

No third-party libraries needed — uses POSIX sockets and `termios` directly.

## Build

```sh
cmake -B build
cmake --build build
```

## Run

```sh
./build/hermes-pi <serial-device> [port]
# Example:
./build/hermes-pi /dev/ttyACM0 5005
```

Default port is `5005`. The Pico must be connected and enumerated as `/dev/ttyACM0` (or the path you specify) before launching.

## Behavior

- **Non-blocking UDP receive** — never stalls waiting for control packets.
- **CRC validation** — packets with wrong magic byte or bad checksum are silently discarded (both directions).
- **Dropped packet logging** — control packet sequence number gaps are printed to `stderr`.
- **Feedback relay** — after each control-packet forward, the bridge reads from serial (non-blocking) and sends any available `FeedbackPacket` back to the PC via UDP on port `5006`. The PC's address is learned automatically from the first received control packet.
- **Serial auto-reconnect** — if a serial write or read fails (e.g. Pico disconnects), the port is closed and reopened on the next attempt; the Pi process does not need to be restarted.
- Serial port configured as **115200 8N1**, raw mode (no terminal processing).
