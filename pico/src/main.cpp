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

enum class DriveMode { ACKERMANN, PIVOT, CRAB, BRAKE };

static RoverController* rover       = nullptr;
static DriveMode        mode        = DriveMode::ACKERMANN;
static uint8_t          prevActions = 0;
static uint8_t          fbSeq       = 0;
static uint32_t         lastPacketMs = 0;

// If no valid ControlPacket arrives within this window (PC exit, crash, USB
// or network disconnect, ...), the rover would otherwise keep driving at its
// last commanded duty cycle forever. ~15 missed cycles at 50 Hz.
static constexpr uint32_t WATCHDOG_TIMEOUT_MS = 300;

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

    lastPacketMs = millis();

    // Rising-edge detect on TOGGLE_PIVOT / CRAB_MODE / BRAKE to switch drive
    // mode. Any button switches directly between modes; if multiple edges
    // land in the same packet, BRAKE wins, then pivot, then crab (deterministic
    // safety-first tie-break).
    bool pivotEdge = (pkt.actions & action::TOGGLE_PIVOT) != 0u &&
                      (prevActions & action::TOGGLE_PIVOT) == 0u;
    bool crabEdge  = (pkt.actions & action::CRAB_MODE) != 0u &&
                      (prevActions & action::CRAB_MODE) == 0u;
    bool brakeEdge = (pkt.actions & action::BRAKE) != 0u &&
                      (prevActions & action::BRAKE) == 0u;
    prevActions = pkt.actions;

    if (brakeEdge) {
        mode = (mode == DriveMode::BRAKE) ? DriveMode::ACKERMANN : DriveMode::BRAKE;
        rover->setTraction(0.0f); // zero traction on any mode change before re-steering
    } else if (pivotEdge) {
        mode = (mode == DriveMode::PIVOT) ? DriveMode::ACKERMANN : DriveMode::PIVOT;
        rover->setTraction(0.0f);
    } else if (crabEdge) {
        mode = (mode == DriveMode::CRAB) ? DriveMode::ACKERMANN : DriveMode::CRAB;
        rover->setTraction(0.0f);
    }

    rover->setBrake(mode == DriveMode::BRAKE);

    switch (mode) {
        case DriveMode::PIVOT:
            rover->setPivot(pkt.throttle);
            break;
        case DriveMode::CRAB:
            rover->setCrab(pkt.steer);
            rover->setTraction(pkt.throttle);
            break;
        case DriveMode::BRAKE:
            break;
        case DriveMode::ACKERMANN:
        default:
            rover->setAckermann(pkt.steer);
            rover->setTraction(pkt.throttle);
            break;
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

    if (millis() - lastPacketMs > WATCHDOG_TIMEOUT_MS) {
        rover->setTraction(0.0f);
    }

    rover->loop();
    sendFeedback();
    delay(20); // 50 Hz control cycle
}
