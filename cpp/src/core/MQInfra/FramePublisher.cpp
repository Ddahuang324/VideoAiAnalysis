#include "core/MQInfra/FramePublisher.h"

#include <string>
#include <zmq.hpp>

#include "Log.h"
#include "core/MQInfra/Protocol.h"

namespace MQInfra {

bool FramePublisher::initialize(const std::string& endpoint) {
    try {
        publisher_ = zmq::socket_t(context_, zmq::socket_type::pub);
        publisher_.bind(endpoint);
    } catch (const zmq::error_t& e) {
        LOG_ERROR("Failed to create or bind ZMQ publisher socket: " + std::string(e.what()));
        return false;
    }
    return true;
}

bool FramePublisher::publish(const Protocol::FrameMessage& frame) {
    if (!Protocol::sendFrameMessageZeroCopy(publisher_, frame)) {
        LOG_ERROR("Failed to send frame message");
        stats_.total_dropped_frames += 1;
        return false;
    }

    stats_.total_sent_frames += 1;
    return true;
}

Stats FramePublisher::getStats() const {
    return stats_;
}

void FramePublisher::shutdown() {
    publisher_.close();
    context_.close();
}
}  // namespace MQInfra