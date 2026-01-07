#include "core/MQInfra/FrameSubscriber.h"

#include <optional>
#include <string>

#include "Log.h"
#include "core/MQInfra/Protocol.h"
#include "zmq.hpp"

namespace MQInfra {
bool FrameSubscriber::initialize(const std::string& endpoint) {
    try {
        subscriber_ = zmq::socket_t(context_, zmq::socket_type::sub);
        subscriber_.connect(endpoint);
        subscriber_.set(zmq::sockopt::subscribe, "");  // Subscribe to all messages
    } catch (const zmq::error_t& e) {
        LOG_ERROR("Failed to create or connect ZMQ subscriber socket: " + std::string(e.what()));
        return false;
    }
    return true;
}

std::optional<Protocol::FrameMessage> FrameSubscriber::receiveFrame(int timeout_ms) {
    auto result = Protocol::receiveFrameMessageZeroCopy(subscriber_, timeout_ms);

    if (!result) {
        stats_.timeout_count += 1;
        return std::nullopt;
    }

    stats_.total_received_frames += 1;
    return result;
}

Stats FrameSubscriber::getStats() const {
    return stats_;
}

void FrameSubscriber::shutdown() {
    subscriber_.close();
    context_.close();
}
}  // namespace MQInfra