#include "core/MQInfra/KeyFrameMetaDataPublisher.h"

#include <cstdint>
#include <string>
#include <vector>

#include "Log.h"
#include "Protocol.h"
#include "zmq.hpp"

namespace MQInfra {
bool KeyFrameMetaDataPublisher::initialize(const std::string& endpoint) {
    try {
        publisher_ = zmq::socket_t(context_, zmq::socket_type::push);
        publisher_.bind(endpoint);
    } catch (const zmq::error_t& e) {
        LOG_ERROR("Failed to create or bind ZMQ push socket: " + std::string(e.what()));
        return false;
    }
    return true;
}

bool KeyFrameMetaDataPublisher::publish(const Protocol::KeyFrameMetaDataMessage& meta) {
    try {
        std::vector<uint8_t> buffer = Protocol::serializeKeyFrameMetaDataMessage(meta);
        publisher_.send(zmq::buffer(buffer), zmq::send_flags::none);
    } catch (const zmq::error_t& e) {
        LOG_ERROR("Failed to send KeyFrameMetaData message: " + std::string(e.what()));
        return false;
    }
    return true;
}

bool KeyFrameMetaDataPublisher::sendStopAck(uint32_t processedCount) {
    Protocol::StopAckHeader header;
    header.processedCount = processedCount;
    // Magic set by default

    try {
        publisher_.send(zmq::buffer(&header, sizeof(header)), zmq::send_flags::none);
        LOG_INFO("Sent STOP_ACK with processedCount: " + std::to_string(processedCount));
        return true;
    } catch (const zmq::error_t& e) {
        LOG_ERROR("Failed to send STOP_ACK: " + std::string(e.what()));
        return false;
    }
}

void KeyFrameMetaDataPublisher::shutdown() {
    publisher_.close();
    context_.close();
}

}  // namespace MQInfra