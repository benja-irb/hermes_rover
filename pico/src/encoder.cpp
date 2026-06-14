/**
 * @file encoder.cpp
 * @brief AS5600 four-channel encoder reader via TCA9548A I2C mux.
 */

#include "encoder.hpp"
#include <Arduino.h>
#include <cmath>

namespace hermes {

Encoder::Encoder() {
    Wire1.setSDA(26);
    Wire1.setSCL(27);
    Wire1.begin();
    Wire1.setClock(100000);

    for (int i = 0; i < 4; ++i) {
        angles[i] = 90.0f;
    }
}

void Encoder::update() {
    for (uint8_t ch = 0; ch < 4; ++ch) {
        float raw = readRaw(ch);
        if (raw < 0.0f) continue; // I2C error on this channel — keep previous value

        // Channels 1-3 are physically mounted in reverse
        if (ch >= 1) {
            raw = fmodf(360.0f - raw, 360.0f);
        }

        // Apply mechanical zero offset; fmodf with +720 guards against negative input
        float cal = fmodf(raw - OFFSETS[ch] + 720.0f, 360.0f);

        // Map the range (270°, 360°) → (−90°, 0°) for continuity near straight-ahead
        if (cal > 270.0f) cal -= 360.0f;

        angles[ch] = cal;
    }
}

float Encoder::readRaw(uint8_t channel) {
    // Select mux channel
    Wire1.beginTransmission(MUX_ADDR);
    Wire1.write(static_cast<uint8_t>(1u << channel));
    if (Wire1.endTransmission() != 0) return -1.0f;

    // Point encoder to angle register
    Wire1.beginTransmission(ENC_ADDR);
    Wire1.write(ANGLE_REG);
    if (Wire1.endTransmission(false) != 0) return -1.0f; // repeated start

    // Read 2 bytes: low nibble of first byte = angle[11:8], second byte = angle[7:0]
    if (Wire1.requestFrom(static_cast<uint8_t>(ENC_ADDR),
                          static_cast<uint8_t>(2)) < 2) return -1.0f;

    uint8_t hi = static_cast<uint8_t>(Wire1.read());
    uint8_t lo = static_cast<uint8_t>(Wire1.read());
    uint16_t adc = (static_cast<uint16_t>(hi & 0x0Fu) << 8) | lo;

    return (static_cast<float>(adc) * 360.0f) / 4096.0f;
}

} // namespace hermes
