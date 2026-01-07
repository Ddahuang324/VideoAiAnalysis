#include "core/MQInfra/KeyFrameMetaDataSubscriber.h"

#include <cstdint>
#include <exception>
#include <optional>
#include <string>
#include <vector>
#include <zmq.hpp>

#include "Protocol.h"

namespace MQInfra {
bool KeyFrameMetaDataSubscriber::initialize(const std::string& endpoint) {
    try {
        subscriber_ = zmq::socket_t(context_, zmq::socket_type::pull);
        subscriber_.connect(endpoint);
    } catch (const zmq::error_t& e) {
        // Handle error (e.g., log it)
        return false;
    }
    return true;
}

std::optional<Protocol::KeyFrameMetaDataMessage> KeyFrameMetaDataSubscriber::receiveMetaData(
    int timeout_ms) {
    subscriber_.set(zmq::sockopt::rcvtimeo, timeout_ms);
    zmq::message_t message;

    try {
        auto result = subscriber_.recv(message, zmq::recv_flags::none);
        if (!result) {
            return std::nullopt;
        }

        std::vector<uint8_t> buffer(static_cast<const uint8_t*>(message.data()),
                                    static_cast<const uint8_t*>(message.data()) + message.size());
        return Protocol::deserializeKeyFrameMetaDataMessage(buffer);

    } catch (const zmq::error_t& e) {
        return std::nullopt;
    } catch (const std::exception& e) {
        return std::nullopt;
    }
}

void KeyFrameMetaDataSubscriber::shutdown() {
    try {
        subscriber_.close();
    } catch (...) {
    }
}
}  // namespace MQInfra