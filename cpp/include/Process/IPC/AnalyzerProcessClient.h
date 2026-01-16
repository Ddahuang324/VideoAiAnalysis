#pragma once

#include <string>

#include "CommandProtocol.h"
#include "IIPCClientBase.h"
#include "nlohmann/json_fwd.hpp"

namespace IPC {

/**
 * @brief 分析进程客户端，用于向分析进程发送控制命令
 */
class AnalyzerProcessClient : public IIPCClientBase {
public:
    explicit AnalyzerProcessClient(const std::string& endpoint) : IIPCClientBase(endpoint) {}

    /**
     * @brief 启动分析算法
     */
    CommandResponse startAnalyzer() { return sendCommand(CommandType::START_ANALYZER); }

    /**
     * @brief 设置分析配置
     * @param config JSON 格式的配置项
     */
    CommandResponse setConfig(const nlohmann::json& config) {
        return sendCommand(CommandType::ANALYZER_CONFIG_SET, config);
    }

    /**
     * @brief 获取分析状态
     */
    CommandResponse getStatus() { return sendCommand(CommandType::GET_STATUS); }

    /**
     * @brief 获取统计信息（如 OCR 识别成功率、延迟等）
     */
    CommandResponse getStats() { return sendCommand(CommandType::GET_STATS); }
};

}  // namespace IPC
