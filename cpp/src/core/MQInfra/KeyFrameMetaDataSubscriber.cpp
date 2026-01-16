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

KeyFrameMetaDataSubscriber::MetaDataReceiveResult KeyFrameMetaDataSubscriber::receive(
    int timeout_ms) {
    MetaDataReceiveResult result{MetaDataReceiveResult::Type::None};
    subscriber_.set(zmq::sockopt::rcvtimeo, timeout_ms);
    zmq::message_t message;

    try {
        auto res = subscriber_.recv(message, zmq::recv_flags::none);
        if (!res) {
            return result;  // None
        }

        // Check for Stop Ack
        if (message.size() == sizeof(Protocol::StopAckHeader)) {
            Protocol::StopAckHeader ack;
            std::memcpy(&ack, message.data(), sizeof(Protocol::StopAckHeader));
            if (ack.magic_num == Protocol::STOP_ACK_MAGIC) {
                result.type = MetaDataReceiveResult::Type::StopAck;
                result.stopAck = ack;
                return result;
            }
        }

        // Try deserialize metadata
        std::vector<uint8_t> buffer(static_cast<const uint8_t*>(message.data()),
                                    static_cast<const uint8_t*>(message.data()) + message.size());

        try {
            auto meta = Protocol::deserializeKeyFrameMetaDataMessage(buffer);
            result.type = MetaDataReceiveResult::Type::MetaData;
            result.metadata = meta;
        } catch (...) {
            result.type = MetaDataReceiveResult::Type::Error;
        }

    } catch (...) {
        result.type = MetaDataReceiveResult::Type::Error;
    }

    return result;
}

void KeyFrameMetaDataSubscriber::shutdown() {
    try {
        subscriber_.close();
    } catch (...) {
    }
}
}  // namespace MQInfra