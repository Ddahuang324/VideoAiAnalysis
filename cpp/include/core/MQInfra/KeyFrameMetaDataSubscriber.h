#pragma once
#include <optional>
#include <string>
#include <zmq.hpp>

#include "Protocol.h"

namespace MQInfra {
class KeyFrameMetaDataSubscriber {
public:
    struct MetaDataReceiveResult {
        enum class Type { MetaData, StopAck, None, Error };
        Type type;
        std::optional<Protocol::KeyFrameMetaDataMessage> metadata;
        std::optional<Protocol::StopAckHeader> stopAck;
    };

    bool initialize(const std::string& endpoint = "tcp://localhost:5556");
    std::optional<Protocol::KeyFrameMetaDataMessage> receiveMetaData(int timeout_ms);
    MetaDataReceiveResult receive(int timeout_ms);
    void shutdown();

private:
    zmq::context_t context_;
    zmq::socket_t subscriber_;
};
}  // namespace MQInfra
