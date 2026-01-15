#include "DynamicCalculator.h"

#include <algorithm>
#include <numeric>
#include <vector>

#include "IFrameAnalyzer.h"
#include "Log.h"

namespace KeyFrame {

// ========== Constructor ==========

DynamicCalculator::DynamicCalculator(const Config& config) : config_(config) {
    // Initialize currentWeights_ immediately to prevent empty vector access
    currentWeights_ = config_.baseWeights.size() >= 3 ? config_.baseWeights
                                                      : std::vector<float>{0.45f, 0.2f, 0.35f};
    historyAverages_.assign(3, 0.0f);
    runningSum_.assign(3, 0.0f);
}

// ========== Weight Normalization ==========

std::vector<float> DynamicCalculator::normaliseWeights(const std::vector<float>& rawWeights) {
    float sum = std::accumulate(rawWeights.begin(), rawWeights.end(), 0.0f);

    if (sum < 1e-6f) {
        LOG_WARN("[DynamicCalculator] Sum of weights too small, using base weights");
        if (config_.baseWeights.size() >= 3) {
            return config_.baseWeights;
        }
        return {0.45f, 0.2f, 0.35f};
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
    currentWeights_ = config_.baseWeights.size() >= 3 ? config_.baseWeights
                                                      : std::vector<float>{0.45f, 0.2f, 0.35f};
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
        currentWeights_ = config_.baseWeights.size() >= 3 ? config_.baseWeights
                                                          : std::vector<float>{0.45f, 0.2f, 0.35f};
    }

    // Update history and running sum
    historyScores_.push_back(currentScores);

    // Safety check: ensure runningSum_ has 3 elements
    if (runningSum_.size() >= 3 && currentScores.size() >= 3) {
        for (size_t i = 0; i < 3; ++i) {
            runningSum_[i] += currentScores[i];
        }
    } else {
        LOG_ERROR("[DynamicCalculator] Vector size mismatch in update - runningSum_: " +
                  std::to_string(runningSum_.size()) +
                  ", currentScores: " + std::to_string(currentScores.size()));
        return ActivationStats{};
    }

    // Maintain window size
    if (historyScores_.size() > static_cast<size_t>(config_.historyWindowSize)) {
        const auto& oldest = historyScores_.front();
        // Safety check: ensure oldest has 3 elements
        if (oldest.size() >= 3) {
            for (size_t i = 0; i < 3; ++i) {
                runningSum_[i] -= oldest[i];
            }
        } else {
            LOG_WARN("[DynamicCalculator] History score vector size mismatch: " +
                     std::to_string(oldest.size()) + ", expected 3");
        }
        historyScores_.pop_front();
    }

    // Compute O(1) moving average
    float invSize = 1.0f / static_cast<float>(historyScores_.size());
    if (historyAverages_.size() >= 3 && runningSum_.size() >= 3) {
        for (size_t i = 0; i < 3; ++i) {
            historyAverages_[i] = runningSum_[i] * invSize;
        }
    } else {
        LOG_ERROR(
            "[DynamicCalculator] Vector size mismatch computing averages - historyAverages_: " +
            std::to_string(historyAverages_.size()) +
            ", runningSum_: " + std::to_string(runningSum_.size()));
        return ActivationStats{};
    }

    // Compute dynamic weights: activation-based adjustment
    // Formula: DynamicWeight = BaseWeight * (1 + beta * Activation)
    // Where: Activation = alpha * CurrentScore + (1 - alpha) * HistoryAvg
    float alpha = config_.currentFrameWeight;
    float beta = config_.activationInfluence;

    // Ensure baseWeights has at least 3 elements
    std::vector<float> safeBaseWeights = config_.baseWeights.size() >= 3
                                             ? config_.baseWeights
                                             : std::vector<float>{0.45f, 0.2f, 0.35f};

    std::vector<float> newWeight(3, 0.0f);

    // Safety check before accessing vectors
    if (currentScores.size() >= 3 && historyAverages_.size() >= 3 && safeBaseWeights.size() >= 3) {
        for (int i = 0; i < 3; ++i) {
            float activation = alpha * currentScores[i] + (1.0f - alpha) * historyAverages_[i];
            newWeight[i] = safeBaseWeights[i] * (1.0f + beta * activation);
        }
    } else {
        LOG_ERROR("[DynamicCalculator] Vector size mismatch computing weights - currentScores: " +
                  std::to_string(currentScores.size()) +
                  ", historyAverages_: " + std::to_string(historyAverages_.size()) +
                  ", safeBaseWeights: " + std::to_string(safeBaseWeights.size()));
        return ActivationStats{};
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
