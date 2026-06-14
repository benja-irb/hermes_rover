/**
 * @file encoder.hpp
 * @brief Four-channel AS5600 magnetic encoder reader via TCA9548A I2C mux.
 *
 * Hardware:
 *   I2C bus 1 — SDA = GP26, SCL = GP27 @ 100 kHz
 *   Mux  0x70 (TCA9548A) — channel selected by writing (1 << ch)
 *   Encoder 0x36 (AS5600) — 12-bit angle at register 0x0C (2 bytes, big-endian)
 *
 * Wheel index convention: 0=FL, 1=FR, 2=RL, 3=RR.
 */

#pragma once

#include <Arduino.h>
#include <Wire.h>

namespace hermes {

class Encoder {
public:
    Encoder();

    /**
     * @brief Read all four channels and refresh angles[].
     *
     * Silently skips a channel if the I2C transaction fails.
     */
    void update();

    /** Calibrated wheel angles [°].  Index: [FL, FR, RL, RR]. */
    float angles[4];

private:
    static constexpr uint8_t MUX_ADDR  = 0x70;
    static constexpr uint8_t ENC_ADDR  = 0x36;
    static constexpr uint8_t ANGLE_REG = 0x0C;

    // Mechanical zero offsets measured at 90° (straight-ahead) position.
    static constexpr float OFFSETS[4] = {193.0f, 233.5f, 123.8f, 293.4f};

    /** Read raw angle [°] from the encoder on the given mux channel. */
    float readRaw(uint8_t channel);
};

} // namespace hermes
