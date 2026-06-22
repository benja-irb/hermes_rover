/**
 * @file udp_server.hpp
 * @brief Non-blocking UDP server that receives ControlPacket datagrams from the PC.
 */

#pragma once

#include <cstdint>
#include <netinet/in.h>
#include "protocol.hpp"

namespace hermes {

/**
 * @brief Binds a UDP socket and validates incoming ControlPacket datagrams.
 *
 * Frames that fail magic/CRC validation are silently discarded.
 */
class UdpServer {
public:
    /**
     * @param port UDP port to listen on (default: HERMES_DEFAULT_PORT).
     * @throws std::runtime_error if the socket cannot be created or bound.
     */
    explicit UdpServer(uint16_t port = DEFAULT_PORT);
    ~UdpServer();

    UdpServer(const UdpServer&) = delete;
    UdpServer& operator=(const UdpServer&) = delete;

    /**
     * @brief Try to receive a validated packet without blocking.
     * @param[out] pkt Populated on success.
     * @return true if a valid packet was received, false otherwise.
     *
     * Also records the sender's address for use by sendFeedback().
     */
    bool receive(ControlPacket& pkt);

    /**
     * @brief Send a FeedbackPacket to the last known PC address on FEEDBACK_PORT.
     * @return false if no sender has been seen yet or the send fails.
     */
    bool sendFeedback(const FeedbackPacket& pkt);

private:
    int         fd_;
    sockaddr_in senderAddr_;
    bool        hasSender_ = false;
};

} // namespace hermes
