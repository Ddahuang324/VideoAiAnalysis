#pragma once

#include <memory>
#include <string>
#include <zmq.hpp>

#include "CommandProtocol.h"
#include "nlohmann/json_fwd.hpp"

namespace IPC {

class IIPCClientBase {
public:
    explicit IIPCClientBase(const std::string& endpoint);
    virtual ~IIPCClientBase();

    bool connect(int timeout_ms = 5000);

    void disconnect();

    CommandResponse sendCommand(const CommandRequest& request, int timeout_ms = 5000);

    bool isConnected() const { return connected_; }

protected:
    CommandResponse sendCommand(CommandType type, const nlohmann::json& params = {});

private:
    std::string endpoint_;
    zmq::context_t context_;
    std::unique_ptr<zmq::socket_t> socket_;
    bool connected_{false};
};
}  // namespace IPC