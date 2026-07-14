/**
 * @file main.cpp
 * @brief Hermes PC client — Xbox controller → UDP → Raspberry Pi.
 *
 * Usage:
 *   ./hermes-pc <pi-ip> [udp-port]
 *
 * Example:
 *   ./hermes-pc 192.168.1.50 5005
 *
 * Reads the Xbox controller via SDL2 at 50 Hz and transmits a ControlPacket
 * UDP datagram to the Pi bridge on each iteration.
 */

#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <csignal>
#include <chrono>
#include <thread>

#include "controller.hpp"
#include "udp_client.hpp"
#include "protocol.hpp"

using namespace hermes;
using namespace std::chrono;

static volatile bool running = true;

static void handleSignal(int) {
    running = false;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::fprintf(stderr, "Usage: %s <pi-ip> [udp-port]\n", argv[0]);
        return EXIT_FAILURE;
    }

    const char*    piIp = argv[1];
    const uint16_t port = (argc >= 3) ? static_cast<uint16_t>(std::atoi(argv[2]))
                                      : DEFAULT_PORT;

    std::signal(SIGINT,  handleSignal);
    std::signal(SIGTERM, handleSignal);

    Controller           controller;
    UdpClient            udp(piIp, port);
    UdpFeedbackReceiver  feedback;

    std::printf("[hermes-pc] sending to %s:%u  feedback on :%u  (Ctrl-C to quit)\n",
                piIp, port, FEEDBACK_PORT);

    ControlPacket  pkt{};
    FeedbackPacket fb{};
    uint8_t        seq = 0;

    constexpr auto PERIOD = milliseconds(20); // 50 Hz

    while (running) {
        auto tick = steady_clock::now();

        if (!controller.poll(pkt, seq)) break; // SDL_QUIT

        if (!udp.send(pkt)) {
            std::fprintf(stderr, "[hermes-pc] UDP send failed\n");
        } else {
            std::printf("\r[tx] thr=%+.2f str=%+.2f act=0x%02X seq=%3u   ",
                        pkt.throttle, pkt.steer, pkt.actions, pkt.seq);
            std::fflush(stdout);
        }

        if (feedback.receive(fb)) {
            std::printf("\n[rx] angles FL=%.1f FR=%.1f RL=%.1f RR=%.1f"
                        "  duty FL=%.0f FR=%.0f RL=%.0f RR=%.0f\n",
                        fb.steer_angles[0], fb.steer_angles[1],
                        fb.steer_angles[2], fb.steer_angles[3],
                        fb.traction_duty[0], fb.traction_duty[1],
                        fb.traction_duty[2], fb.traction_duty[3]);
        }

        std::this_thread::sleep_until(tick + PERIOD);
    }

    // Best-effort final stop: the rover has no idea we've quit, so send a
    // few zero-throttle/zero-steer packets to guard against UDP packet loss
    // on this last, un-retried command.
    ControlPacket stopPkt{};
    stopPkt.magic    = MAGIC;
    stopPkt.throttle = 0.0f;
    stopPkt.steer    = 0.0f;
    stopPkt.aux      = 0.0f;
    stopPkt.actions  = 0u;
    for (int i = 0; i < 3; ++i) {
        stopPkt.seq = seq++;
        stampCrc(stopPkt);
        udp.send(stopPkt);
        std::this_thread::sleep_for(milliseconds(20));
    }

    std::printf("\n[hermes-pc] bye.\n");
    return EXIT_SUCCESS;
}
