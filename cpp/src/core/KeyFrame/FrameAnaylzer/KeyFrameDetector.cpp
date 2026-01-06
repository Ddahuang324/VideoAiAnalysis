#include "KeyFrameDetector.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <vector>

#include "FrameScorer.h"

namespace KeyFrame {

bool KeyFrameDetector::checkTemporalConstraint(double timestamp,
                                               std::vector<double>& selectedTimestamps) {
    if (selectedTimestamps.empty())
        return true;

    // 检查与所有已选帧的最小距离（由于 candidates 是按分数排的，时间戳是乱序插入的）
    for (double t : selectedTimestamps) {
        if (std::abs(timestamp - t) < config_.minTemporalDistance) {
            return false;
        }
    }
    return true;
}

std::vector<FrameScore> KeyFrameDetector::preFilter(const std::vector<FrameScore> scores) {
    std::vector<FrameScore> filtered;
    for (const auto& score : scores) {
        if (score.finalscore >= config_.minScoreThreshold ||
            score.rawScores.sceneChangeResult.isSceneChange) {
            filtered.push_back(score);
        }
    }
    return filtered;
}

KeyFrameDetector::SelectionResult KeyFrameDetector::selectFromFrames(
    const std::vector<FrameScore>& frameScores) {
    SelectionResult result;
    result.totalFrames = frameScores.empty() ? 0 : frameScores.back().frameIndex + 1;

    // 1. 预过滤
    auto candidates = preFilter(frameScores);

    // 2. 按分数从高到低排序
    std::sort(candidates.begin(), candidates.end(),
              [](const FrameScore& a, const FrameScore& b) { return a.finalscore > b.finalscore; });

    // 3. 贪心选择
    std::vector<double> selectedTimestamps;
    std::vector<FrameScore> selectedScores;

    for (const auto& cand : candidates) {
        // 如果已经达到目标数量，停止（除非是强制要求的场景切换）
        if (selectedScores.size() >= static_cast<size_t>(config_.targetKeyFrameCount)) {
            if (!cand.rawScores.sceneChangeResult.isSceneChange)
                continue;
        }

        // 检查时间约束
        if (checkTemporalConstraint(cand.timestamp, selectedTimestamps)) {
            selectedScores.push_back(cand);
            selectedTimestamps.push_back(cand.timestamp);
            // 移除内部排序，因为 candidates 是按分数降序的，
            // 我们只需要保证 selectedTimestamps 在 checkTemporalConstraint 中能正确工作
            // 如果 checkTemporalConstraint 依赖有序，我们应该手动维护有序性
        }
    }

    // 4. 整理结果
    for (const auto& s : selectedScores) {
        result.keyFrameIndices.push_back(s.frameIndex);
        result.KeyframeScores.push_back(s);
    }

    // 最终按索引排序，确保输出是按时间顺序的
    std::sort(result.keyFrameIndices.begin(), result.keyFrameIndices.end());

    result.selectedFrames = result.keyFrameIndices.size();
    result.achievedCompressionRatio =
        static_cast<float>(result.selectedFrames) / result.totalFrames;

    return result;
}

}  // namespace KeyFrame
