#pragma once
#include <cstdint>
#include <optional>
#include <string>
#include <zmq.hpp>

#include "Protocol.h"

namespace MQInfra {

struct SubscriberStats {
    uint64_t total_received_frames;  // 4 bytes 总接收帧数
    uint64_t crc_error_frames;       // 4 bytes CRC错误帧数
    double timeout_count;            // 8 bytes 超时帧数
};
class FrameSubscriber {
public:
    FrameSubscriber() = default;
    ~FrameSubscriber() = default;

    bool initialize(const std::string& endpoint = "tcp://localhost:5555");

    std::optional<Protocol::FrameMessage> receiveFrame(int timeout_ms = 100);

    SubscriberStats getStats() const;
    void shutdown();

private:
    zmq::context_t context_;
    zmq::socket_t subscriber_;
    SubscriberStats stats_{0, 0, 0.0};
};

}  // namespace MQInfra