#include "DynamicCalculator.h"

#include <algorithm>
#include <cstddef>
#include <numeric>
#include <vector>

#include "IFrameAnalyzer.h"
#include "Log.h"

namespace KeyFrame {

std::vector<float> DynamicCalculator::normaliseWeights(const std::vector<float>& rawWeights) {
    float sum = std::accumulate(rawWeights.begin(), rawWeights.end(), 0.0f);

    // 避免除以零
    if (sum < 1e-6f) {
        LOG_WARN("Sum of weights is too small, returning equal weights.");
        return config_.baseWeights;
    }

    std::vector<float> normalizedWeights = rawWeights;
    for (float& weight : normalizedWeights) {
        weight /= sum;
        weight = std::clamp(weight, config_.minWeight, config_.maxWeight);
    }
    return normalizedWeights;
}

void DynamicCalculator::reset() {
    historyScores_.clear();
    historyAverages_.assign(3, 0.0f);
    runningSum_.assign(3, 0.0f);
    currentWeights_ = config_.baseWeights;
}

DynamicCalculator::ActivationStats DynamicCalculator::update(const MultiDimensionScore& scores) {
    std::vector<float> currentScores = {scores.sceneScore, scores.motionScore, scores.textScore};

    // 初始化成员变量 (如果为空)
    if (runningSum_.empty()) {
        runningSum_.assign(3, 0.0f);
    }
    if (historyAverages_.empty()) {
        historyAverages_.assign(3, 0.0f);
    }
    if (currentWeights_.empty()) {
        currentWeights_ = config_.baseWeights;
    }

    // 更新历史分数记录和累加和
    historyScores_.push_back(currentScores);
    for (size_t i = 0; i < 3; ++i) {
        runningSum_[i] += currentScores[i];
    }

    // 保持历史记录大小不超过窗口大小
    if (historyScores_.size() > static_cast<size_t>(config_.historyWindowSize)) {
        const auto& oldest = historyScores_.front();
        for (size_t i = 0; i < 3; ++i) {
            runningSum_[i] -= oldest[i];
        }
        historyScores_.pop_front();
    }

    // O(1) 计算历史平均值
    float invSize = 1.0f / static_cast<float>(historyScores_.size());
    for (size_t i = 0; i < 3; ++i) {
        historyAverages_[i] = runningSum_[i] * invSize;
    }

    // 计算当前帧的动态权重
    float alpha = config_.currentFrameWeight;  // α
    float beta = config_.activationInfluence;  // β

    // 计算激活度和动态调整权重
    /**
     *@brief 动态权重计算公式，根据每一个权重在当前帧和历史帧的表现进行动态的激活调整
     */
    std::vector<float> newWeight(3, 0.0f);
    for (int i = 0; i < 3; ++i) {
        // 激活度= α * CurrentScore + (1 - α) * HistoryAvg
        float activation = alpha * currentScores[i] + (1.0f - alpha) * historyAverages_[i];

        // 动态调整权重 = BaseWeight * (1 + β * Activation)
        newWeight[i] = config_.baseWeights[i] * (1.0f + beta * activation);
    }

    // 归一化并限制到 [minWeight, maxWeight]
    currentWeights_ = normaliseWeights(newWeight);

    ActivationStats stats;
    stats.activations = currentScores;       // 当前的分数
    stats.dynamicWeights = currentWeights_;  // 当前计算的动态权重
    stats.histroyAvg = historyAverages_;     // 历史平均分数
    return stats;
}
}  // namespace KeyFrame