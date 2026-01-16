#include "Process/IPC/IPCServer.h"

#include <chrono>
#include <exception>
#include <memory>
#include <string>
#include <thread>
#include <utility>

#include "CommandProtocol.h"
#include "Log.h"
#include "zmq.hpp"

namespace IPC {

IPCServer::IPCServer(const std::string& endpoint)
    : endpoint_(endpoint), context_(), socket_(nullptr), running_(false) {}

IPCServer::~IPCServer() {
    stop();
}

void IPCServer::registerHandler(CommandType command, CommandHandler handler) {
    handlers_[command] = std::move(handler);
    LOG_INFO("Registered handler for command: " + commandTypeToString(command) +
             " at endpoint: " + endpoint_);
}

bool IPCServer::start() {
    if (running_) {
        LOG_WARN("IPCServer is already running at endpoint: " + endpoint_);
        return false;
    }

    LOG_INFO("Starting IPCServer at endpoint: " + endpoint_);

    try {
        socket_ = std::make_unique<zmq::socket_t>(context_, zmq::socket_type::rep);
        socket_->bind(endpoint_);
        running_ = true;
        serverThread_ = std::thread(&IPCServer::serverLoop, this);

        LOG_INFO("IPCServer successfully started and listening at: " + endpoint_);
        return true;
    } catch (const zmq::error_t& e) {
        LOG_ERROR("Failed to start IPCServer at endpoint '" + endpoint_ +
                  "': " + std::string(e.what()) + " (errno: " + std::to_string(e.num()) + ")");
        running_ = false;
        socket_.reset();
        return false;
    } catch (const std::exception& e) {
        LOG_ERROR("Unexpected exception while starting IPCServer at endpoint '" + endpoint_ +
                  "': " + std::string(e.what()));
        running_ = false;
        socket_.reset();
        return false;
    }
}

void IPCServer::stop() {
    if (!running_) {
        LOG_DEBUG("IPCServer stop requested but server is not running at endpoint: " + endpoint_);
        return;
    }

    LOG_INFO("Stopping IPCServer at endpoint: " + endpoint_);
    running_ = false;

    try {
        if (serverThread_.joinable()) {
            LOG_DEBUG("Waiting for server thread to join at endpoint: " + endpoint_);
            serverThread_.join();
            LOG_DEBUG("Server thread joined successfully at endpoint: " + endpoint_);
        }

        if (socket_) {
            LOG_DEBUG("Closing socket at endpoint: " + endpoint_);
            socket_->close();
            socket_.reset();
            LOG_DEBUG("Socket closed successfully at endpoint: " + endpoint_);
        }

        LOG_INFO("IPCServer stopped successfully at endpoint: " + endpoint_);
    } catch (const std::exception& e) {
        LOG_ERROR("Exception during IPCServer shutdown at endpoint '" + endpoint_ +
                  "': " + std::string(e.what()));
    }
}

void IPCServer::serverLoop() {
    while (running_) {
        try {
            zmq::message_t request;
            // 等待接收请求
            auto result = socket_->recv(request, zmq::recv_flags::dontwait);

            if (!result) {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                continue;  // 没有收到消息，继续等待
            }

            // 解析请求
            std::string requestStr(reinterpret_cast<const char*>(request.data()), request.size());
            CommandRequest cmdRequest = CommandRequest::deserialize(requestStr);

            // 处理请求
            auto response = handleRequest(cmdRequest);

            // 发送响应
            std::string responseStr = response.serialize();
            zmq::message_t responseMsg(responseStr.data(), responseStr.size());
            socket_->send(responseMsg, zmq::send_flags::none);

        } catch (const zmq::error_t& e) {
            LOG_ERROR("IPCServer zmq error: " + std::string(e.what()));
        }
    }
}

CommandResponse IPCServer::handleRequest(const CommandRequest& request) {
    CommandResponse response;

    auto it = handlers_.find(request.command);
    if (it != handlers_.end()) {
        try {
            response = it->second(request);
        } catch (const std::exception& e) {
            response.code = ResponseCode::ERROR_INTERNAL;
            response.message = "Handler exception: " + std::string(e.what());
            LOG_ERROR("Handler exception for command " + commandTypeToString(request.command) +
                      ": " + e.what());
        }
    } else {
        response.code = ResponseCode::ERROR_INVALID_CMD;
        response.message = "No handler registered for command";
        LOG_WARN("No handler for command " + commandTypeToString(request.command));
    }
    return response;
}
}  // namespace IPC