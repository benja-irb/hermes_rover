/**
 * @file serial_bridge.hpp
 * @brief Serial port writer that forwards ControlPacket frames to the Pico.
 *
 * Configures the port as 115200 8N1 in raw mode (no terminal processing).
 * If a write fails (cable unplugged), the port is closed and re-opened on
 * the next send() call.
 */

#pragma once

#include <string>
#include "protocol.hpp"

namespace hermes {

class SerialBridge {
public:
    /**
     * @param device Path to the serial device, e.g. "/dev/ttyACM0".
     * @throws std::runtime_error if the port cannot be opened.
     */
    explicit SerialBridge(const std::string& device);
    ~SerialBridge();

    SerialBridge(const SerialBridge&) = delete;
    SerialBridge& operator=(const SerialBridge&) = delete;

    /**
     * @brief Write a ControlPacket to the serial port.
     *
     * Attempts to reconnect if the port was lost.  Returns false if the write
     * ultimately fails after one reconnect attempt.
     */
    bool send(const ControlPacket& pkt);

    /**
     * @brief Try to read a FeedbackPacket from the Pico without blocking.
     *
     * Scans for the FEEDBACK_MAGIC byte, reads the remaining frame, and
     * validates the CRC.  Returns false if no valid packet is available yet.
     */
    bool receive(FeedbackPacket& pkt);

private:
    std::string device_;
    int         fd_;

    // Accumulation buffer for incoming FeedbackPacket bytes
    uint8_t rxBuf_[sizeof(FeedbackPacket)];
    size_t  rxLen_ = 0;

    bool open();
    void close();
};

} // namespace hermes
