/**
 * @file serial_bridge.cpp
 * @brief Serial bridge to Pico via USB-CDC (POSIX termios).
 */

#include "serial_bridge.hpp"
#include "debug.hpp"

#include <stdexcept>
#include <cstring>
#include <cerrno>
#include <cstdio>

#include <fcntl.h>
#include <unistd.h>
#include <termios.h>

namespace hermes {

SerialBridge::SerialBridge(const std::string& device)
    : device_(device), fd_(-1), rxLen_(0)
{
    if (!open()) {
        throw std::runtime_error("Cannot open serial port: " + device_);
    }
}

SerialBridge::~SerialBridge() {
    close();
}

bool SerialBridge::open() {
    fd_ = ::open(device_.c_str(), O_RDWR | O_NOCTTY | O_NONBLOCK);
    if (fd_ < 0) return false;

    // Configure 115200 8N1, raw mode (no terminal processing)
    termios tty{};
    tcgetattr(fd_, &tty);   // start from current port state
    cfmakeraw(&tty);        // clear all processing flags
    cfsetispeed(&tty, B115200);
    cfsetospeed(&tty, B115200);
    tty.c_cflag |= CREAD | CLOCAL;
    tty.c_cc[VMIN]  = 0;
    tty.c_cc[VTIME] = 0;

    if (tcsetattr(fd_, TCSANOW, &tty) < 0) {
        ::close(fd_);
        fd_ = -1;
        return false;
    }

    return true;
}

void SerialBridge::close() {
    if (fd_ >= 0) {
        ::close(fd_);
        fd_ = -1;
    }
}

bool SerialBridge::receive(FeedbackPacket& pkt) {
    if (fd_ < 0) return false;

    // Drain whatever bytes are available into the accumulation buffer
    ssize_t n = ::read(fd_, rxBuf_ + rxLen_, sizeof(rxBuf_) - rxLen_);
    if (n > 0) rxLen_ += static_cast<size_t>(n);

    // Discard leading bytes that are not the magic byte
    size_t discarded = 0;
    while (rxLen_ > 0 && rxBuf_[0] != FEEDBACK_MAGIC) {
        std::memmove(rxBuf_, rxBuf_ + 1, rxLen_ - 1);
        --rxLen_;
        ++discarded;
    }
    if (discarded > 0) {
        DBG("[pi-serial] discarded %zu byte(s) searching for magic 0x%02X",
            discarded, FEEDBACK_MAGIC);
    }

    // Wait until we have a full frame
    if (rxLen_ < sizeof(FeedbackPacket)) return false;

    memcpy(&pkt, rxBuf_, sizeof(FeedbackPacket));

    // Always consume the first byte so the next call advances past this frame
    std::memmove(rxBuf_, rxBuf_ + sizeof(FeedbackPacket), rxLen_ - sizeof(FeedbackPacket));
    rxLen_ -= sizeof(FeedbackPacket);

    if (!validateCrc(pkt)) {
        DBG("%s", "[pi-serial] rx FeedbackPacket CRC fail — dropped");
        return false;
    }
    DBG("[pi-serial] rx FeedbackPacket seq=%u angles=%.1f %.1f %.1f %.1f",
        pkt.seq,
        pkt.steer_angles[0], pkt.steer_angles[1],
        pkt.steer_angles[2], pkt.steer_angles[3]);
    return true;
}

bool SerialBridge::send(const ControlPacket& pkt) {
    // Attempt one reconnect if the port is not open
    if (fd_ < 0 && !open()) return false;

    ssize_t n = ::write(fd_, &pkt, sizeof(pkt));
    if (n == static_cast<ssize_t>(sizeof(pkt))) {
        DBG("[pi-serial] tx ControlPacket seq=%u thr=%+.2f str=%+.2f",
            pkt.seq, pkt.throttle, pkt.steer);
        return true;
    }

    // Write failed — assume disconnect; close and signal failure
    std::fprintf(stderr, "[serial] write failed (%s), reconnecting...\n",
                 strerror(errno));
    close();
    return false;
}

} // namespace hermes
