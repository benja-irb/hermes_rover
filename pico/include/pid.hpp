/**
 * @file pid.hpp
 * @brief Discrete-time PID controller for wheel-angle servo loops.
 *
 * Constants tuned for field operation on the Hermes rover:
 *   Kp = 3.5, Ki = 12.0, Kd = 1.0
 *
 * Features:
 *   - ±1° dead zone — returns 0 and clears state to avoid micro-oscillation
 *   - Integral anti-windup: clamped to ±20
 *   - Minimum output magnitude ramp to prevent motor stall near target
 *   - Output clamped to ±100
 */

#pragma once

namespace hermes {

class PID {
public:
    PID();

    /**
     * @param setpoint Desired wheel angle [°].
     * @param measured Current angle from encoder [°].
     * @return Control effort in [-100.0, 100.0].
     */
    float compute(float setpoint, float measured);

    void reset();

private:
    static constexpr float KP         = 3.5f;
    static constexpr float KI         = 12.0f;
    static constexpr float KD         = 1.0f;
    static constexpr float DEAD_ZONE  = 1.0f;
    static constexpr float INTG_CLAMP = 20.0f;
    static constexpr float OUT_CLAMP  = 100.0f;

    float         integral_;
    float         prevErr_;
    unsigned long lastUs_; // timestamp of last compute() call (microseconds)
};

} // namespace hermes
