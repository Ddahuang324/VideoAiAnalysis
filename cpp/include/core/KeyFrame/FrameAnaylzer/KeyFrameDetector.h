#pragma once

#include <memory>
#include <string>
#include <vector>

#include "FrameScorer.h"
#include "IFrameAnalyzer.h"

namespace KeyFrame {

class KeyFrameDetector {
public:
    struct Config {
        int targetKeyFrameCount = 50;         // 目标关键帧数量
        float targetCompressionRatio = 0.1f;  // 目标压缩比

        float minTemporalDistance = 1.0f;  // 最小时间间隔（秒）

        float minScoreThreshold = 0.3f;         // 最小分数阈值
        bool alwaysIncludeSceneChanges = true;  // 始终包含场景变化帧
    };

    struct SelectionResult {
        std::vector<int> keyFrameIndices;        // 选中的关键帧索引
        std::vector<FrameScore> KeyframeScores;  // 关键帧对应的分数

        int totalFrames;                 // 总帧数
        int selectedFrames;              // 选中的关键帧数
        float achievedCompressionRatio;  // 实际压缩比
        float averageTemporalDistance;   // 平均时间间隔
    };

    explicit KeyFrameDetector(const Config& config) : config_(config) {}

    // 从视频中解码，并选择关键帧
    SelectionResult selectKeyFrames(const std::string& videopath);

    // 从预先计算的帧分数中选择关键帧
    SelectionResult selectFromFrames(const std::vector<FrameScore>& frameScores);

    const Config& getConfig() const { return config_; }

private:
    /**
     * @brief: 预过滤帧分数，移除低分帧
     * @param scores: 输入的帧分数列表
     * @return: 过滤后的帧分数列表
     */
    std::vector<FrameScore> preFilter(const std::vector<FrameScore> scores);
    /**
     * @brief: 检查时间约束，确保关键帧间隔合理
     * @param timestamp: 当前帧时间戳
     * @param selectedTimestamps: 已选关键帧时间戳列表
     * @return: 是否满足时间约束
     */
    bool checkTemporalConstraint(double timestamp, std::vector<double>& selectedTimestamps);
    Config config_;
    std::shared_ptr<IFrameAnalyzer> analyzer_;
    std::shared_ptr<FrameScorer> scorer_;
};
}  // namespace KeyFrame