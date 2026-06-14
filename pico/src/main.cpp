/**
 * @file main.cpp
 * @brief Hermes Pico firmware entry point.
 *
 * Reads 16-byte ControlPacket frames from USB-CDC serial (sent by the Pi bridge),
 * dispatches motion commands to the RoverController, and runs the PID steering loop
 * at 50 Hz.
 *
 * Serial framing:
 *   - Sync by scanning for the 0xAB magic byte
 *   - Read remaining 15 bytes
 *   - Validate XOR checksum
 *   - Discard frame on validation failure (no retry)
 */

#include <Arduino.h>
#include <cstring>
#include "protocol.hpp"
#include "rover_controller.hpp"

using namespace hermes;

static RoverController* rover       = nullptr;
static bool             tankMode    = false;
static uint8_t          prevActions = 0;
static uint8_t          fbSeq       = 0;

// ---------------------------------------------------------------------------
// Packet ingestion
// ---------------------------------------------------------------------------

static void tryReadPacket() {
    // Drain bytes until we are aligned to a magic byte
    while (Serial.available() > 0 &&
           static_cast<uint8_t>(Serial.peek()) != MAGIC)
    {
        Serial.read();
    }

    if (Serial.available() < static_cast<int>(sizeof(ControlPacket))) return;

    uint8_t buf[sizeof(ControlPacket)];
    size_t n = Serial.readBytes(reinterpret_cast<char*>(buf), sizeof(ControlPacket));
    if (n < sizeof(ControlPacket)) return;

    ControlPacket pkt;
    memcpy(&pkt, buf, sizeof(ControlPacket));

    if (!validateCrc(pkt)) return;

    // Rising-edge detect on TOGGLE_PIVOT to switch between Ackermann and pivot mode
    bool pivot     = (pkt.actions & action::TOGGLE_PIVOT) != 0u;
    bool prevPivot = (prevActions & action::TOGGLE_PIVOT) != 0u;
    if (pivot && !prevPivot) {
        tankMode = !tankMode;
        if (!tankMode) {
            // Return wheels to straight and stop when leaving pivot mode
            rover->setTraction(0.0f);
        }
    }
    prevActions = pkt.actions;

    if (tankMode) {
        rover->setPivot(pkt.throttle);
    } else {
        rover->setAckermann(pkt.steer);
        rover->setTraction(pkt.throttle);
    }
}

// ---------------------------------------------------------------------------
// Arduino entry points
// ---------------------------------------------------------------------------

void setup() {
    Serial.begin(BAUD_RATE);

    // Global PWM configuration — must be called before any analogWrite
    analogWriteFreq(1000);   // 1 kHz for all H-bridge channels
    analogWriteRange(65535); // 16-bit duty cycle resolution

    // Give USB CDC time to enumerate on the host
    delay(2000);

    rover = new RoverController();
}

static void sendFeedback() {
    FeedbackPacket fb{};
    fb.magic = FEEDBACK_MAGIC;
    fb.seq   = fbSeq++;
    memcpy(fb.steer_angles,   rover->steerAngles(),   sizeof(fb.steer_angles));
    memcpy(fb.traction_duty,  rover->tractionDuty(),  sizeof(fb.traction_duty));
    stampCrc(fb);
    Serial.write(reinterpret_cast<const uint8_t*>(&fb), sizeof(fb));
}

void loop() {
    tryReadPacket();
    rover->loop();
    sendFeedback();
    delay(20); // 50 Hz control cycle
}
