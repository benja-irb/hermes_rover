/**
 * @file rover_controller.hpp
 * @brief Top-level rover state machine: traction, Ackermann steering, and PID loop.
 *
 * Wheel index convention throughout: 0=FL, 1=FR, 2=RL, 3=RR.
 *
 * Traction motor pin pairs:
 *   FL(0,1)  FR(2,3)  RL(4,5)  RR(6,7)
 *
 * Steering motor pin pairs:
 *   FL(8,9)  FR(12,13)  RL(10,11)  RR(14,15)
 */

#pragma once

#include "motor.hpp"
#include "encoder.hpp"
#include "pid.hpp"

namespace hermes {

class RoverController {
public:
    RoverController();

    /** Read encoders and run one PID iteration for all steering motors. */
    void loop();

    /**
     * @brief Drive all four traction motors at the given normalized speed.
     * @param val [-1.0, 1.0]. Dead zone |val| < 0.05 maps to stop.
     */
    void setTraction(float val);

    /**
     * @brief Compute Ackermann steering angles and update PID targets.
     * @param val [-1.0, 1.0]. Dead zone |val| < 0.05 resets to straight.
     */
    void setAckermann(float val);

    /**
     * @brief Pivot (spin-in-place) mode.
     *
     * Sets fixed wheel angles for a symmetric pivot and drives diagonal motor pairs.
     * @param speed [-1.0, 1.0]. Positive = clockwise.
     */
    void setPivot(float speed);

    /** Stop all motors and release PWM resources. */
    void stopAll();

    /** Last applied traction PWM duty per wheel [−100, 100]. FL, FR, RL, RR. */
    const float* tractionDuty() const { return tDuty_; }

    /** Current measured steering angles [°]. FL, FR, RL, RR. */
    const float* steerAngles() const { return enc_.angles; }

private:
    Motor tMots_[4]; // traction motors, index = [FL, FR, RL, RR]
    Motor sMots_[4]; // steering motors, index = [FL, FR, RL, RR]
    Encoder enc_;
    PID pids_[4];

    float targets_[4]; // PID setpoints [°], default 90° = straight ahead
    float tDuty_[4];   // last applied traction duty per wheel

    // Per-wheel traction inversion: compensates for physically mirrored mounting.
    static constexpr int8_t T_INV[4] = {1, -1, -1, 1};

    // Ackermann geometry (metres)
    static constexpr float L_HALF = 0.35f; // half-wheelbase
    static constexpr float W_HALF = 0.25f; // half-track-width
    static constexpr float R_MIN  = 0.6f;  // minimum turning radius at full deflection

    static constexpr float TRACTION_DEAD = 0.05f;
    static constexpr float STEER_DEAD    = 0.05f;
};

} // namespace hermes
