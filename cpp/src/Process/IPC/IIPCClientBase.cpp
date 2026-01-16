#include "Process/IPC/IIPCClientBase.h"

#include <memory>
#include <string>

#include "CommandProtocol.h"
#include "Log.h"
#include "nlohmann/json_fwd.hpp"
#include "zmq.h"
#include "zmq.hpp"

namespace IPC {

IIPCClientBase::IIPCClientBase(const std::string& endpoint)
    : endpoint_(endpoint), context_(1), socket_(nullptr), connected_(false) {}

IIPCClientBase::~IIPCClientBase() {
    disconnect();
}

bool IIPCClientBase::connect(int timeout_ms) {
    try {
        socket_ = std::make_unique<zmq::socket_t>(context_, zmq::socket_type::req);
        socket_->setsockopt(ZMQ_RCVTIMEO, timeout_ms);
        socket_->connect(endpoint_);
        connected_ = true;
        LOG_INFO("Successfully connected to IPC server at endpoint: " + endpoint_);
        return true;
    } catch (const zmq::error_t& e) {
        connected_ = false;
        LOG_ERROR("Failed to connect to IPC server at endpoint '" + endpoint_ +
                  "': " + std::string(e.what()));
        return false;
    }
}

void IIPCClientBase::disconnect() {
    if (socket_) {
        socket_->close();
        socket_.reset();
    }
    connected_ = false;
    LOG_INFO("Disconnected from IPC server at endpoint: " + endpoint_);
}

CommandResponse IIPCClientBase::sendCommand(const CommandRequest& request, int timeout_ms) {
    if (!connected_ || !socket_) {
        LOG_ERROR("IIPCClientBase not connected to endpoint: " + endpoint_);
        return CommandResponse::createErrorResponse(request.command, "Not connected to IPC server");
    }

    try {
        std::string requestStr = request.serialize();
        zmq::message_t requestMsg(requestStr.data(), requestStr.size());

        // 设置发送超时
        socket_->setsockopt(ZMQ_SNDTIMEO, timeout_ms);
        // 确保接收超时也是最新的
        socket_->setsockopt(ZMQ_RCVTIMEO, timeout_ms);
        auto send_result = socket_->send(requestMsg, zmq::send_flags::none);
        if (!send_result) {
            LOG_ERROR("Failed to send command to IPC server at endpoint: " + endpoint_);
            return CommandResponse::createErrorResponse(request.command, "Failed to send command");
        }

        zmq::message_t reply;
        auto recv_result = socket_->recv(reply, zmq::recv_flags::none);
        if (!recv_result) {
            LOG_ERROR("Timeout waiting for response from IPC server at endpoint: " + endpoint_);
            return CommandResponse::createErrorResponse(request.command,
                                                        "Timeout waiting for response");
        }

        std::string replyStr(reinterpret_cast<const char*>(reply.data()), reply.size());
        return CommandResponse::deserialize(replyStr);
    } catch (const zmq::error_t& e) {
        LOG_ERROR("IIPCClientBase zmq error: " + std::string(e.what()));
        // 如果发生错误，重置连接状态，因为 REQ-REP 状态可能已损坏
        connected_ = false;
        return CommandResponse::createErrorResponse(request.command,
                                                    "ZMQ error: " + std::string(e.what()));
    }
}

CommandResponse IIPCClientBase::sendCommand(CommandType type, const nlohmann::json& params) {
    CommandRequest request;
    request.command = type;
    request.parameters = params;
    return sendCommand(request);
}
}  // namespace IPC