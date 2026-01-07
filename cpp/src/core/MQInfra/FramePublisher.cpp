#include "core/MQInfra/FramePublisher.h"

#include <cstddef>
#include <cstdint>
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
    return publishRaw(frame.header, frame.image_data.data(), frame.image_data.size(), frame.crc32);
}

bool FramePublisher::publishRaw(const Protocol::FrameHeader& header, const void* data,
                                size_t data_size, uint32_t crc) {
    if (!Protocol::sendFrameRawZeroCopy(publisher_, header, data, data_size, crc)) {
        LOG_ERROR("Failed to send raw frame message");
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