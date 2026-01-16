#pragma once

#include <nlohmann/json.hpp>
#include <string>

#include "nlohmann/json_fwd.hpp"

namespace IPC {

enum class CommandType {
    // 基本命令
    PING,
    GET_STATUS,
    GET_STATS,
    SHUTDOWN,
    // 录制控制命令
    START_RECORDING,
    STOP_RECORDING,
    PAUSE_RECORDING,
    RESUME_RECORDING,
    // 进程控制命令
    START_ANALYZER,
    STOP_ANALYZER,
    START_RECORDER,
    ANALYZER_CONFIG_SET,

    UNKNOWN
};

enum class ResponseCode {
    SUCCESS = 0,
    ERROR_INVALID_CMD = 1,
    ERROR_INVALID_PRMS = 2,
    ERROR_NOT_INITIALIZED = 3,
    ERROR_ALREADY_RUNNING = 4,
    ERROR_NOT_RUNNING = 5,
    ERROR_INTERNAL = 6
};

// 命令请求结构体
struct CommandRequest {
    CommandType command;
    nlohmann::json parameters;

    std::string serialize() const;
    static CommandRequest deserialize(const std::string& str);
};

// 命令响应结构体
struct CommandResponse {
    ResponseCode code;
    std::string message;
    nlohmann::json data;

    std::string serialize() const;
    static CommandResponse deserialize(const std::string& str);
    static CommandResponse createErrorResponse(CommandType command, const std::string& message,
                                               ResponseCode code = ResponseCode::ERROR_INTERNAL);
};

CommandType stringToCommandType(const std::string& str);
std::string commandTypeToString(CommandType type);
}  // namespace IPC