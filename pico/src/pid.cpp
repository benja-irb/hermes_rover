/**
 * @file pid.cpp
 * @brief Discrete-time PID controller implementation.
 */

#include "pid.hpp"
#include <Arduino.h>
#include <cmath>

namespace hermes {

PID::PID()
    : integral_(0.0f), prevErr_(0.0f), lastUs_(micros())
{}

float PID::compute(float setpoint, float measured) {
    unsigned long now = micros();
    // Unsigned subtraction is overflow-safe (wraps correctly after ~70 min)
    float dt = static_cast<float>(now - lastUs_) / 1e6f;
    if (dt < 0.001f) dt = 0.001f;
    lastUs_ = now;

    float err = setpoint - measured;

    // Dead zone: avoid chasing small residual errors
    if (fabsf(err) <= DEAD_ZONE) {
        integral_ = 0.0f;
        prevErr_  = 0.0f;
        return 0.0f;
    }

    // Anti-windup integral clamp
    integral_ += err * dt;
    if (integral_ >  INTG_CLAMP) integral_ =  INTG_CLAMP;
    if (integral_ < -INTG_CLAMP) integral_ = -INTG_CLAMP;

    float derivative = (err - prevErr_) / dt;
    prevErr_ = err;

    float out = KP * err + KI * integral_ + KD * derivative;

    // Minimum output magnitude: ramps to 25 as error approaches 5°,
    // preventing motor stall while the wheel closes in on target.
    float cMin;
    if (fabsf(err) < 5.0f) {
        cMin = fmaxf(25.0f * (fabsf(err) / 5.0f), 10.0f);
    } else {
        cMin = 25.0f;
    }

    if (out > 0.0f && out < cMin)  out = cMin;
    if (out < 0.0f && out > -cMin) out = -cMin;

    // Final output clamp
    if (out >  OUT_CLAMP) out =  OUT_CLAMP;
    if (out < -OUT_CLAMP) out = -OUT_CLAMP;

    return out;
}

void PID::reset() {
    integral_ = 0.0f;
    prevErr_  = 0.0f;
    lastUs_   = micros();
}

} // namespace hermes
