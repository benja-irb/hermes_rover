/**
 * @file main.cpp
 * @brief Hermes Pi bridge — UDP receiver → USB serial forwarder.
 *
 * Usage:
 *   ./hermes-pi <serial-device> [udp-port]
 *
 * Example:
 *   ./hermes-pi /dev/ttyACM0 5005
 *
 * The bridge receives ControlPacket datagrams from the PC and forwards each
 * validated packet over USB serial to the Pico.  Sequence number gaps are
 * logged to stderr as an indicator of network loss.
 */

#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <csignal>

#include "udp_server.hpp"
#include "serial_bridge.hpp"
#include "protocol.hpp"

using namespace hermes;

static volatile bool running = true;

static void handleSignal(int) {
    running = false;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::fprintf(stderr, "Usage: %s <serial-device> [udp-port]\n", argv[0]);
        return EXIT_FAILURE;
    }

    const char*    device = argv[1];
    const uint16_t port   = (argc >= 3) ? static_cast<uint16_t>(std::atoi(argv[2]))
                                        : DEFAULT_PORT;

    std::signal(SIGINT,  handleSignal);
    std::signal(SIGTERM, handleSignal);

    UdpServer    udp(port);
    SerialBridge serial(device);

    std::printf("[hermes-pi] listening on UDP :%u  →  %s\n", port, device);

    ControlPacket pkt{};
    FeedbackPacket fb{};
    uint8_t       expectedSeq = 0;
    bool          seqInitialized = false;

    while (running) {
        if (udp.receive(pkt)) {
            // Log dropped packets (sequence number gaps)
            if (seqInitialized && pkt.seq != expectedSeq) {
                std::fprintf(stderr, "[hermes-pi] seq gap: expected %u got %u\n",
                             expectedSeq, pkt.seq);
            }
            expectedSeq    = static_cast<uint8_t>(pkt.seq + 1u);
            seqInitialized = true;

            serial.send(pkt);
        }

        if (serial.receive(fb)) {
            udp.sendFeedback(fb);
        }
    }

    std::printf("\n[hermes-pi] shutting down.\n");
    return EXIT_SUCCESS;
}
