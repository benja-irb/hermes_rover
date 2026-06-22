/**
 * @file udp_server.cpp
 * @brief UDP server implementation (POSIX).
 */

#include "udp_server.hpp"
#include "debug.hpp"

#include <stdexcept>
#include <cstring>
#include <cerrno>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>

namespace hermes {

UdpServer::UdpServer(uint16_t port) : fd_(-1) {
    fd_ = ::socket(AF_INET, SOCK_DGRAM, 0);
    if (fd_ < 0) {
        throw std::runtime_error(std::string("socket(): ") + strerror(errno));
    }

    // Non-blocking so receive() never stalls the bridge loop
    ::fcntl(fd_, F_SETFL, O_NONBLOCK);

    sockaddr_in addr{};
    addr.sin_family      = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port        = htons(port);

    if (::bind(fd_, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
        ::close(fd_);
        throw std::runtime_error(std::string("bind(): ") + strerror(errno));
    }
}

UdpServer::~UdpServer() {
    if (fd_ >= 0) ::close(fd_);
}

bool UdpServer::receive(ControlPacket& pkt) {
    uint8_t  buf[sizeof(ControlPacket)];
    socklen_t addrLen = sizeof(senderAddr_);
    ssize_t n = ::recvfrom(fd_, buf, sizeof(buf), 0,
                            reinterpret_cast<sockaddr*>(&senderAddr_), &addrLen);

    if (n != static_cast<ssize_t>(sizeof(ControlPacket))) return false;

    memcpy(&pkt, buf, sizeof(ControlPacket));
    if (!validateCrc(pkt)) {
        DBG("%s", "[pi-udp] rx ControlPacket CRC fail — dropped");
        return false;
    }

    char fromIp[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &senderAddr_.sin_addr, fromIp, sizeof(fromIp));
    DBG("[pi-udp] rx ControlPacket seq=%u thr=%+.2f str=%+.2f act=0x%02X  from %s",
        pkt.seq, pkt.throttle, pkt.steer, pkt.actions, fromIp);

    hasSender_ = true;
    return true;
}

bool UdpServer::sendFeedback(const FeedbackPacket& pkt) {
    if (!hasSender_) {
        DBG("%s", "[pi-udp] sendFeedback skipped — no sender learned yet");
        return false;
    }

    sockaddr_in dest  = senderAddr_;
    dest.sin_port     = htons(FEEDBACK_PORT);

    ssize_t n = ::sendto(fd_,
                          reinterpret_cast<const char*>(&pkt), sizeof(pkt),
                          0,
                          reinterpret_cast<const sockaddr*>(&dest), sizeof(dest));
    bool ok = n == static_cast<ssize_t>(sizeof(pkt));
    DBG("[pi-udp] tx FeedbackPacket seq=%u → port %u  %s",
        pkt.seq, FEEDBACK_PORT, ok ? "ok" : "FAILED");
    return ok;
}

} // namespace hermes
