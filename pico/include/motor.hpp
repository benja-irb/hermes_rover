/**
 * @file motor.hpp
 * @brief H-bridge motor driver using dual-pin PWM (RP2040 Arduino framework).
 *
 * Each motor channel is controlled by two GPIO pins — one for each direction.
 * Both pins low = coast; one pin high = directional drive.
 *
 * PWM frequency and range must be configured globally before constructing any
 * Motor instance:
 *   analogWriteFreq(1000);   // 1 kHz
 *   analogWriteRange(65535); // 16-bit resolution
 */

#pragma once

#include <Arduino.h>

namespace hermes {

class Motor {
public:
    /**
     * @param pinPos GPIO for positive direction.
     * @param pinNeg GPIO for negative direction.
     */
    Motor(uint8_t pinPos, uint8_t pinNeg);

    /**
     * @brief Drive the motor.
     * @param duty Signed [-100.0, 100.0]. Positive = forward, negative = reverse.
     */
    void setDuty(float duty);

    /** Brake to zero and release PWM. */
    void deinit();

private:
    uint8_t pinPos_;
    uint8_t pinNeg_;
};

} // namespace hermes
