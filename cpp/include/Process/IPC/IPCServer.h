#pragma once

#include <atomic>
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <thread>
#include <zmq.hpp>

#include "CommandProtocol.h"

namespace IPC {

class IPCServer {
public:
    using CommandHandler = std::function<CommandResponse(const CommandRequest& request)>;

    IPCServer(const std::string& endpoint);
    ~IPCServer();

    bool start();
    void stop();

    bool isRunning() { return running_; }
    void registerHandler(CommandType command, CommandHandler handler);

private:
    void serverLoop();
    CommandResponse handleRequest(const CommandRequest& request);
    std::string endpoint_;
    zmq::context_t context_;
    std::unique_ptr<zmq::socket_t> socket_;
    std::map<CommandType, CommandHandler> handlers_;
    std::thread serverThread_;
    std::atomic<bool> running_{false};
};
}  // namespace IPC