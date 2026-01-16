#pragma once

#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "core/Config/UnifiedConfig.h"

namespace Analyzer {

// 分析状态枚举
enum class AnalysisStatus { IDLE, INITIALIZING, RUNNING, STOPPING, ERROR };

// 分析模式枚举
enum class AnalysisMode {
    REALTIME,  // ZMQ实时传输，适合SNAPSHOT模式（1FPS）
    OFFLINE    // 离线分析，适合VIDEO模式或批量处理
};

// 关键帧简要记录
struct KeyFrameRecord {
    int64_t frameIndex;
    float score;
    double timestamp;
};

// 分析统计信息与状态快照
struct AnalysisStats {
    int64_t receivedFrameCount = 0;  // 接收帧数
    int64_t analyzedFrameCount = 0;  // 已分析帧数
    int64_t keyframeCount = 0;       // 关键帧总数

    // 最近捕获的关键帧列表 (保持最近 N 个)
    std::vector<KeyFrameRecord> latestKeyFrames;

    // 当前运行配置摘要
    struct ConfigSummary {
        bool textRecognitionEnabled = false;
        int threadCount = 0;
        std::string activeModelInfo;
    } activeConfig;

    double avgProcessingTime = 0.0;  // 平均处理时间（ms）
};

// 使用统一配置系统的类型别名
using AnalyzerConfig = Config::KeyFrameAnalyzerConfig;

class AnalyzerAPI {
public:
    AnalyzerAPI();

    ~AnalyzerAPI();

    // 生命周期管理
    bool initialize(const AnalyzerConfig& config);

    bool start();

    // 异步启动视频文件离线分析
    bool analyzeVideoFile(const std::string& filePath);

    bool stop();

    void shutdown();

    // 状态查询
    AnalysisStatus getStatus() const;

    AnalysisStats getStats() const;

    std::string getLastError() const;

    // 回调设置
    using StatusCallback = std::function<void(AnalysisStatus)>;

    using KeyFrameCallback = std::function<void(int64_t frameIndex)>;

    void setStatusCallback(StatusCallback callback);

    void setKeyFrameCallback(KeyFrameCallback callback);

    using KeyFrameVideoCallback = std::function<void(const std::string& videoPath)>;

    void setKeyFrameVideoCallback(KeyFrameVideoCallback callback);

    // 实时分析控制（适合SNAPSHOT模式）
    bool startRealtimeAnalysis();

    void stopRealtimeAnalysis();

    bool isRealtimeMode() const;

    // 分析模式管理
    void setAnalysisMode(AnalysisMode mode);

    AnalysisMode getAnalysisMode() const;

private:
    class Impl;

    std::unique_ptr<Impl> impl_;
};

}  // namespace Analyzer
