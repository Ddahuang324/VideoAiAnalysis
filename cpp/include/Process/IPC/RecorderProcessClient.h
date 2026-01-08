#pragma once

#include <string>

#include "CommandProtocol.h"
#include "IIPCClientBase.h"
#include "nlohmann/json_fwd.hpp"


namespace IPC {

/**
 * @brief 录制进程客户端，用于向录制进程发送控制命令
 */
class RecorderProcessClient : public IIPCClientBase {
public:
    explicit RecorderProcessClient(const std::string& endpoint) : IIPCClientBase(endpoint) {}

    /**
     * @brief 开始录制
     * @param output_path 录制文件保存路径（可选）
     */
    CommandResponse startRecording(const std::string& output_path = "") {
        nlohmann::json params;
        if (!output_path.empty()) {
            params["output_path"] = output_path;
        }
        return sendCommand(CommandType::START_RECORDING, params);
    }

    /**
     * @brief 停止录制
     */
    CommandResponse stopRecording() { return sendCommand(CommandType::STOP_RECORDING); }

    /**
     * @brief 暂停录制
     */
    CommandResponse pauseRecording() { return sendCommand(CommandType::PAUSE_RECORDING); }

    /**
     * @brief 恢复录制
     */
    CommandResponse resumeRecording() { return sendCommand(CommandType::RESUME_RECORDING); }

    /**
     * @brief 获取录制状态
     */
    CommandResponse getStatus() { return sendCommand(CommandType::GET_STATUS); }
};

}  // namespace IPC
