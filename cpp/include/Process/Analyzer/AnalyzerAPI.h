#pragma once

#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace Analyzer {

// 分析状态枚举

enum class AnalysisStatus { IDLE, INITIALIZING, RUNNING, STOPPING, ERROR };

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

// 分析配置

struct AnalyzerConfig {
    std::string zmqSubscribeEndpoint = "tcp://localhost:5555";

    std::string zmqPublishEndpoint = "tcp://*:5556";

    std::string modelBasePath;

    bool enableTextRecognition = false;

    // 模型路径

    std::string sceneModelPath;

    std::string motionModelPath;

    std::string textDetModelPath;

    std::string textRecModelPath;
};

class AnalyzerAPI {
public:
    AnalyzerAPI();

    ~AnalyzerAPI();

    // 生命周期管理

    bool initialize(const AnalyzerConfig& config);

    bool start();

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

private:
    class Impl;

    std::unique_ptr<Impl> impl_;
};

}  // namespace Analyzer