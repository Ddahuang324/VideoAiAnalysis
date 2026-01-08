#include "KeyFrameDetector.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <vector>

#include "FrameScorer.h"

namespace KeyFrame {

bool KeyFrameDetector::checkTemporalConstraint(double timestamp,
                                               std::vector<double>& selectedTimestamps) {
    // 根据用户反馈，模型和算法层面已很大程度上避免了时序性冗余问题
    // 因此这里不再进行强制的时序间隔检查，直接返回 true。
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
    const std::vector<FrameScore>& frameScores, int dynamicTargetCount) {
    SelectionResult result;
    if (frameScores.empty()) {
        result.totalFrames = 0;
        result.selectedFrames = 0;
        return result;
    }

    // 计算总帧数 (取最后一个的索引+1)
    result.totalFrames = frameScores.back().frameIndex + 1;

    // 1. 预过滤 (移除低于 minScoreThreshold 的帧)
    auto candidates = preFilter(frameScores);

    // 2. 按分数从高到低排序，以优先保证最高质量帧被选中
    std::sort(candidates.begin(), candidates.end(),
              [](const FrameScore& a, const FrameScore& b) { return a.finalscore > b.finalscore; });

    // 3. 核心选择逻辑
    std::vector<double> selectedTimestamps;
    std::vector<FrameScore> selectedScores;

    if (config_.useThresholdMode) {
        // 模式 A: 阈值模式 - 只要分数够高就选，配合时间间隔约束
        for (const auto& cand : candidates) {
            // 达到上限则停止
            if (selectedScores.size() >= static_cast<size_t>(config_.maxKeyFrameCount)) {
                break;
            }

            // 检查分数是否达标
            bool isHighQuality = (cand.finalscore >= config_.highQualityThreshold);
            bool isSceneChange = cand.rawScores.sceneChangeResult.isSceneChange;

            if (isHighQuality || isSceneChange) {
                // 检查时间约束（防止在同一秒内爆发式产生过多帧）
                if (checkTemporalConstraint(cand.timestamp, selectedTimestamps)) {
                    selectedScores.push_back(cand);
                    selectedTimestamps.push_back(cand.timestamp);
                }
            }
        }
    } else {
        // 模式 B: Top-K 模式 - 动态目标数量
        int targetK = (dynamicTargetCount > 0) ? dynamicTargetCount : config_.targetKeyFrameCount;
        // 限制在保底和上限范围内
        targetK = std::max(config_.minKeyFrameCount, std::min(targetK, config_.maxKeyFrameCount));

        for (const auto& cand : candidates) {
            // 达到目标数量 K 则停止（场景切换强制包含除外）
            if (selectedScores.size() >= static_cast<size_t>(targetK)) {
                if (!cand.rawScores.sceneChangeResult.isSceneChange)
                    continue;
            }

            if (checkTemporalConstraint(cand.timestamp, selectedTimestamps)) {
                selectedScores.push_back(cand);
                selectedTimestamps.push_back(cand.timestamp);
            }
        }
    }

    // 4. 整理结果并按索引（时间顺序）回写
    for (const auto& s : selectedScores) {
        result.keyFrameIndices.push_back(s.frameIndex);
        result.KeyframeScores.push_back(s);
    }

    // 最终按索引排序，确保输出是按时间顺序的
    std::sort(result.keyFrameIndices.begin(), result.keyFrameIndices.end());
    // 同时通过索引映射回复原本的分数列表顺序 (如果需要的话)

    result.selectedFrames = result.keyFrameIndices.size();
    result.achievedCompressionRatio =
        static_cast<float>(result.selectedFrames) / result.totalFrames;

    return result;
}

}  // namespace KeyFrame
