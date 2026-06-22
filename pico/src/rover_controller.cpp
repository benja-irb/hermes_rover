/**
 * @file rover_controller.cpp
 * @brief Rover state machine: traction, Ackermann steering, pivot, and PID loop.
 */

#include "rover_controller.hpp"
#include <cmath>

namespace hermes {

static constexpr float PI_F = 3.14159265358979323846f;

RoverController::RoverController()
    : tMots_{Motor(0, 1), Motor(2, 3), Motor(4, 5), Motor(6, 7)},
      sMots_{Motor(8, 9), Motor(12, 13), Motor(10, 11), Motor(14, 15)},
      enc_(),
      pids_{PID(), PID(), PID(), PID()},
      targets_{90.0f, 90.0f, 90.0f, 90.0f},
      tDuty_{0.0f, 0.0f, 0.0f, 0.0f}
{}

void RoverController::loop() {
    enc_.update();
    for (int i = 0; i < 4; ++i) {
        float effort = pids_[i].compute(targets_[i], enc_.angles[i]);
        sMots_[i].setDuty(effort);
    }
}

void RoverController::setTraction(float val) {
    if (fabsf(val) < TRACTION_DEAD) val = 0.0f;
    if (val >  1.0f) val =  1.0f;
    if (val < -1.0f) val = -1.0f;

    for (int i = 0; i < 4; ++i) {
        float adj = val * static_cast<float>(T_INV[i]);
        tDuty_[i] = adj * 100.0f;
        tMots_[i].setDuty(tDuty_[i]);
    }
}

void RoverController::setAckermann(float val) {
    if (val >  1.0f) val =  1.0f;
    if (val < -1.0f) val = -1.0f;

    if (fabsf(val) < STEER_DEAD) {
        for (int i = 0; i < 4; ++i) targets_[i] = 90.0f;
        return;
    }

    float R = R_MIN / fabsf(val);
    float angleIn  = (180.0f / PI_F) * atanf(L_HALF / (R - W_HALF));
    float angleOut = (180.0f / PI_F) * atanf(L_HALF / (R + W_HALF));

    if (val > 0.0f) { // right turn: inner = right wheels, outer = left wheels
        targets_[0] = 90.0f - angleOut; // FL (outer)
        targets_[1] = 90.0f - angleIn;  // FR (inner)
        targets_[2] = 90.0f + angleOut; // RL (outer)
        targets_[3] = 90.0f + angleIn;  // RR (inner)
    } else {          // left turn: inner = left wheels, outer = right wheels
        targets_[0] = 90.0f + angleIn;  // FL (inner)
        targets_[1] = 90.0f + angleOut; // FR (outer)
        targets_[2] = 90.0f - angleIn;  // RL (inner)
        targets_[3] = 90.0f - angleOut; // RR (outer)
    }
}

void RoverController::setCrab(float val) {
    if (val >  1.0f) val =  1.0f;
    if (val < -1.0f) val = -1.0f;

    if (fabsf(val) < STEER_DEAD) {
        for (int i = 0; i < 4; ++i) targets_[i] = 90.0f;
        return;
    }

    float angle = 90.0f + val * 90.0f;
    for (int i = 0; i < 4; ++i) targets_[i] = angle;
}

void RoverController::setPivot(float speed) {
    // Wheels arranged in an X pattern for symmetric pivot
    targets_[0] =  36.0f; // FL
    targets_[1] = 144.0f; // FR
    targets_[2] = 144.0f; // RL
    targets_[3] =  36.0f; // RR

    if (speed >  1.0f) speed =  1.0f;
    if (speed < -1.0f) speed = -1.0f;

    // Diagonal pairs drive in opposite directions to spin in place
    const float velocities[4] = {-speed, speed, speed, -speed};

    for (int i = 0; i < 4; ++i) {
        float adj = velocities[i] * static_cast<float>(T_INV[i]);
        tDuty_[i] = adj * 100.0f;
        tMots_[i].setDuty(tDuty_[i]);
    }
}

void RoverController::stopAll() {
    setTraction(0.0f);
    for (int i = 0; i < 4; ++i) {
        tMots_[i].deinit();
        sMots_[i].deinit();
    }
}

} // namespace hermes
