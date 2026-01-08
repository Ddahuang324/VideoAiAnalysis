#include "Process/IPC/CommandProtocol.h"

#include <map>
#include <nlohmann/json.hpp>
#include <string>

#include "nlohmann/json_fwd.hpp"

namespace IPC {

std::string CommandRequest::serialize() const {
    nlohmann::json j;
    j["command"] = commandTypeToString(command);
    j["parameters"] = parameters;
    return j.dump();
}

CommandRequest CommandRequest::deserialize(const std::string& str) {
    auto j = nlohmann::json::parse(str);
    CommandRequest req;
    req.command = stringToCommandType(j.value("command", "UNKNOWN"));
    req.parameters = j["parameters"];
    return req;
}

std::string CommandResponse::serialize() const {
    nlohmann::json j;
    j["code"] = static_cast<int>(code);
    j["message"] = message;
    j["data"] = data;
    return j.dump();
}

CommandResponse CommandResponse::deserialize(const std::string& str) {
    auto j = nlohmann::json::parse(str);
    CommandResponse resp;
    resp.code =
        static_cast<ResponseCode>(j.value("code", static_cast<int>(ResponseCode::ERROR_INTERNAL)));
    resp.message = j.value("message", "");
    resp.data = j["data"];
    return resp;
}

CommandResponse CommandResponse::createErrorResponse(CommandType command,
                                                     const std::string& message,
                                                     ResponseCode code) {
    CommandResponse resp;
    resp.code = code;
    resp.message = message;
    return resp;
}

CommandType stringToCommandType(const std::string& str) {
    static const std::map<std::string, CommandType> mapping = {
        {"PING", CommandType::PING},
        {"GET_STATUS", CommandType::GET_STATUS},
        {"GET_STATS", CommandType::GET_STATS},
        {"SHUTDOWN", CommandType::SHUTDOWN},
        {"START_RECORDING", CommandType::START_RECORDING},
        {"STOP_RECORDING", CommandType::STOP_RECORDING},
        {"PAUSE_RECORDING", CommandType::PAUSE_RECORDING},
        {"RESUME_RECORDING", CommandType::RESUME_RECORDING},
        {"START_ANALYZER", CommandType::START_ANALYZER},
        {"STOP_ANALYZER", CommandType::STOP_ANALYZER},
        {"START_RECORDER", CommandType::START_RECORDER},
        {"ANALYZER_CONFIG_SET", CommandType::ANALYZER_CONFIG_SET}};
    auto it = mapping.find(str);
    return (it != mapping.end()) ? it->second : CommandType::UNKNOWN;
}

std::string commandTypeToString(CommandType type) {
    switch (type) {
        case CommandType::PING:
            return "PING";
        case CommandType::GET_STATUS:
            return "GET_STATUS";
        case CommandType::GET_STATS:
            return "GET_STATS";
        case CommandType::SHUTDOWN:
            return "SHUTDOWN";
        case CommandType::START_RECORDING:
            return "START_RECORDING";
        case CommandType::STOP_RECORDING:
            return "STOP_RECORDING";
        case CommandType::PAUSE_RECORDING:
            return "PAUSE_RECORDING";
        case CommandType::RESUME_RECORDING:
            return "RESUME_RECORDING";
        case CommandType::START_ANALYZER:
            return "START_ANALYZER";
        case CommandType::STOP_ANALYZER:
            return "STOP_ANALYZER";
        case CommandType::START_RECORDER:
            return "START_RECORDER";
        case CommandType::ANALYZER_CONFIG_SET:
            return "ANALYZER_CONFIG_SET";
        default:
            return "UNKNOWN";
    }
}
}  // namespace IPC