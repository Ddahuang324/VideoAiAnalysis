#include "DynamicCalculator.h"

#include <algorithm>
#include <numeric>
#include <vector>

#include "IFrameAnalyzer.h"
#include "Log.h"

namespace KeyFrame {

// ========== Weight Normalization ==========

std::vector<float> DynamicCalculator::normaliseWeights(const std::vector<float>& rawWeights) {
    float sum = std::accumulate(rawWeights.begin(), rawWeights.end(), 0.0f);

    if (sum < 1e-6f) {
        LOG_WARN("[DynamicCalculator] Sum of weights too small, using base weights");
        return config_.baseWeights;
    }

    std::vector<float> normalizedWeights = rawWeights;
    for (float& weight : normalizedWeights) {
        weight /= sum;
        weight = std::clamp(weight, config_.minWeight, config_.maxWeight);
    }

    return normalizedWeights;
}

// ========== Reset ==========

void DynamicCalculator::reset() {
    historyScores_.clear();
    historyAverages_.assign(3, 0.0f);
    runningSum_.assign(3, 0.0f);
    currentWeights_ = config_.baseWeights;
}

// ========== Update ==========

DynamicCalculator::ActivationStats DynamicCalculator::update(const MultiDimensionScore& scores) {
    std::vector<float> currentScores = {scores.sceneScore, scores.motionScore, scores.textScore};

    // Initialize if empty
    if (runningSum_.empty()) {
        runningSum_.assign(3, 0.0f);
    }
    if (historyAverages_.empty()) {
        historyAverages_.assign(3, 0.0f);
    }
    if (currentWeights_.empty()) {
        currentWeights_ = config_.baseWeights;
    }

    // Update history and running sum
    historyScores_.push_back(currentScores);
    for (size_t i = 0; i < 3; ++i) {
        runningSum_[i] += currentScores[i];
    }

    // Maintain window size
    if (historyScores_.size() > static_cast<size_t>(config_.historyWindowSize)) {
        const auto& oldest = historyScores_.front();
        for (size_t i = 0; i < 3; ++i) {
            runningSum_[i] -= oldest[i];
        }
        historyScores_.pop_front();
    }

    // Compute O(1) moving average
    float invSize = 1.0f / static_cast<float>(historyScores_.size());
    for (size_t i = 0; i < 3; ++i) {
        historyAverages_[i] = runningSum_[i] * invSize;
    }

    // Compute dynamic weights: activation-based adjustment
    // Formula: DynamicWeight = BaseWeight * (1 + beta * Activation)
    // Where: Activation = alpha * CurrentScore + (1 - alpha) * HistoryAvg
    float alpha = config_.currentFrameWeight;
    float beta = config_.activationInfluence;

    std::vector<float> newWeight(3, 0.0f);
    for (int i = 0; i < 3; ++i) {
        float activation = alpha * currentScores[i] + (1.0f - alpha) * historyAverages_[i];
        newWeight[i] = config_.baseWeights[i] * (1.0f + beta * activation);
    }

    // Normalize and clamp
    currentWeights_ = normaliseWeights(newWeight);

    ActivationStats stats;
    stats.activations = currentScores;
    stats.dynamicWeights = currentWeights_;
    stats.histroyAvg = historyAverages_;

    return stats;
}

}  // namespace KeyFrame
