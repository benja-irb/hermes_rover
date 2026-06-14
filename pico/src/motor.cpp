/**
 * @file motor.cpp
 * @brief H-bridge motor driver implementation.
 */

#include "motor.hpp"
#include <Arduino.h>
#include <cmath>

namespace hermes {

Motor::Motor(uint8_t pinPos, uint8_t pinNeg)
    : pinPos_(pinPos), pinNeg_(pinNeg)
{
    pinMode(pinPos_, OUTPUT);
    pinMode(pinNeg_, OUTPUT);
    analogWrite(pinPos_, 0);
    analogWrite(pinNeg_, 0);
}

void Motor::setDuty(float duty) {
    // Clamp to valid range
    if (duty >  100.0f) duty =  100.0f;
    if (duty < -100.0f) duty = -100.0f;

    // Scale: 100 units → 16-bit full scale (65535)
    uint16_t d = static_cast<uint16_t>(fabsf(duty) * 655.35f);

    if (duty > 0.0f) {
        analogWrite(pinPos_, d);
        analogWrite(pinNeg_, 0);
    } else if (duty < 0.0f) {
        analogWrite(pinPos_, 0);
        analogWrite(pinNeg_, d);
    } else {
        analogWrite(pinPos_, 0);
        analogWrite(pinNeg_, 0);
    }
}

void Motor::deinit() {
    analogWrite(pinPos_, 0);
    analogWrite(pinNeg_, 0);
}

} // namespace hermes
