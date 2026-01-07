#pragma once

#include <cstdint>
#include <string>
#include <zmq.hpp>

#include "Protocol.h"

namespace MQInfra {

struct Stats {
    uint64_t total_sent_frames;
    uint64_t total_dropped_frames;
    double drop_rate;
};

class FramePublisher {
public:
    FramePublisher() = default;
    ~FramePublisher() = default;

    bool initialize(const std::string& endpoint = "tcp://*:5555");

    bool publish(const Protocol::FrameMessage& frame);
    Stats getStats() const;
    void shutdown();

private:
    zmq::context_t context_;
    zmq::socket_t publisher_;
    Stats stats_{0, 0, 0.0};
};
}  // namespace MQInfra