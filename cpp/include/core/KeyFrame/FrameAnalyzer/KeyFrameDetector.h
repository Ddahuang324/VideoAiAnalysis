#pragma once

#include <memory>
#include <string>
#include <vector>

#include "core/Config/UnifiedConfig.h"
#include "FrameScorer.h"
#include "IFrameAnalyzer.h"

namespace KeyFrame {

// 使用统一配置系统的类型别名
using KeyFrameDetectorConfig = Config::KeyFrameDetectorConfig;

class KeyFrameDetector {
public:
    // 使用统一配置
    using Config = KeyFrameDetectorConfig;

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
    // @param dynamicTargetCount: 如果提供且 > 0, 则覆盖配置中的 targetKeyFrameCount
    SelectionResult selectFromFrames(const std::vector<FrameScore>& frameScores,
                                     int dynamicTargetCount = -1);

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
