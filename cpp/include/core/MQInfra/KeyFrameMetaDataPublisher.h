#pragma once

#include <string>
#include <zmq.hpp>

#include "Protocol.h"

namespace MQInfra {

class KeyFrameMetaDataPublisher {
public:
    bool initialize(const std::string& endpoint = "tcp://*:5556");
    bool publish(const Protocol::KeyFrameMetaDataMessage& meta);
    void shutdown();

private:
    zmq::context_t context_;
    zmq::socket_t publisher_;
};
}  // namespace MQInfra