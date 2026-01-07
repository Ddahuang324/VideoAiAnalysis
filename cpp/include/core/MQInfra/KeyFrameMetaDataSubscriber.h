#pragma once
#include <optional>
#include <string>
#include <zmq.hpp>

#include "Protocol.h"

namespace MQInfra {
class KeyFrameMetaDataSubscriber {
public:
    bool initialize(const std::string& endpoint = "tcp://localhost:5556");
    std::optional<Protocol::KeyFrameMetaDataMessage> receiveMetaData(int timeout_ms);
    void shutdown();

private:
    zmq::context_t context_;
    zmq::socket_t subscriber_;
};
}  // namespace MQInfra
