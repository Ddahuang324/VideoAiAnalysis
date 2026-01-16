#include "FrameScorer.h"

#include <algorithm>
#include <cstddef>
#include <memory>
#include <mutex>
#include <string>
#include <utility>
#include <vector>

#include "DynamicCalculator.h"
#include "IFrameAnalyzer.h"
#include "Log.h"

namespace KeyFrame {

// ========== Constructor ==========

FrameScorer::FrameScorer(std::shared_ptr<DynamicCalculator> weightCalculator, const Config& config)
    : weightCalculator_(std::move(weightCalculator)), config_(config) {}

// ========== Score Fusion ==========

float FrameScorer::FuseScores(const MultiDimensionScore& scores,
                              std::vector<float>& appliedWeights) {
    // Get weights (dynamic or default)
    if (config_.enableDynamicWeighting && weightCalculator_) {
        weightCalculator_->update(scores);
        appliedWeights = weightCalculator_->getCurrentWeights();
    } else {
        appliedWeights = {0.45f, 0.2f, 0.35f};  // Default weights
    }

    if (appliedWeights.size() < 3) {
        LOG_WARN("[FrameScorer] Weight vector size mismatch, using defaults");
        appliedWeights = {0.45f, 0.2f, 0.35f};
    }

    float finalScore = scores.sceneScore * appliedWeights[0] +
                       scores.motionScore * appliedWeights[1] +
                       scores.textScore * appliedWeights[2];

    LOG_INFO("[FrameScorer] Final Score: " + std::to_string(finalScore));
    return finalScore;
}

// ========== Main Scoring ==========

FrameScore FrameScorer::score(const MultiDimensionScore& scores, const AnalysisContext& context) {
    std::lock_guard<std::mutex> lock(mutex_);

    FrameScore result;
    result.frameIndex = context.frameIndex;
    result.timestamp = context.timestamp;
    result.rawScores = scores;

    float fused = FuseScores(scores, result.appliedWeights);
    float boosted = ApplyBoosts(fused, scores);
    result.finalscore = ApplyTemporalSmoothing(context.frameIndex, boosted);

    // Record contributions for debugging (with safety check)
    if (result.appliedWeights.size() >= 3) {
        result.sceneContribution = scores.sceneScore * result.appliedWeights[0];
        result.motionContribution = scores.motionScore * result.appliedWeights[1];
        result.textContribution = scores.textScore * result.appliedWeights[2];
    } else {
        LOG_ERROR("[FrameScorer] appliedWeights size is " +
                  std::to_string(result.appliedWeights.size()) + ", expected 3");
        result.sceneContribution = 0.0f;
        result.motionContribution = 0.0f;
        result.textContribution = 0.0f;
    }

    return result;
}

std::vector<FrameScore> FrameScorer::scoreBatch(const std::vector<MultiDimensionScore>& scoresBatch,
                                                const std::vector<AnalysisContext>& contexts) {
    if (scoresBatch.size() != contexts.size()) {
        LOG_ERROR("[FrameScorer] Batch size mismatch");
        return {};
    }

    std::vector<FrameScore> results;
    results.reserve(scoresBatch.size());

    for (size_t i = 0; i < scoresBatch.size(); ++i) {
        results.push_back(score(scoresBatch[i], contexts[i]));
    }

    return results;
}

// ========== Reset ==========

void FrameScorer::reset() {
    std::lock_guard<std::mutex> lock(mutex_);

    while (!scoreHistory_.empty()) {
        scoreHistory_.pop();
    }

    sumScores_ = 0.0f;
    lastSmoothedScore_ = 0.0f;

    if (weightCalculator_) {
        weightCalculator_->reset();
    }
}

// ========== Score Boosts ==========

float FrameScorer::ApplyBoosts(float baseScore, const MultiDimensionScore& scores) {
    float boostedScore = baseScore;

    if (config_.sceneChangeBoost > 1.0f && scores.sceneChangeResult.isSceneChange) {
        boostedScore *= config_.sceneChangeBoost;
        LOG_INFO("[FrameScorer] Scene change boost: " + std::to_string(config_.sceneChangeBoost));
    }

    if (config_.motionIncreaseBoost > 1.0f && scores.motionResult.score > 0.5f) {
        boostedScore *= config_.motionIncreaseBoost;
        LOG_INFO("[FrameScorer] Motion boost: " + std::to_string(config_.motionIncreaseBoost));
    }

    if (config_.textIncreaseBoost > 1.0f && scores.textResult.changeRatio > 0.1f) {
        boostedScore *= config_.textIncreaseBoost;
        LOG_INFO("[FrameScorer] Text boost: " + std::to_string(config_.textIncreaseBoost));
    }

    return std::min(1.0f, boostedScore);
}

// ========== Temporal Smoothing ==========

float FrameScorer::ApplyTemporalSmoothing(int frameIndex, float currentScore) {
    if (!config_.enableSmoothing) {
        return currentScore;
    }

    // EMA (Exponential Moving Average) - preferred for faster response
    if (config_.smoothingEMAAlpha > 0.0f && config_.smoothingEMAAlpha <= 1.0f) {
        if (scoreHistory_.empty()) {
            lastSmoothedScore_ = currentScore;
        } else {
            // EMA: S_t = alpha * X_t + (1 - alpha) * S_{t-1}
            lastSmoothedScore_ = config_.smoothingEMAAlpha * currentScore +
                                 (1.0f - config_.smoothingEMAAlpha) * lastSmoothedScore_;
        }

        scoreHistory_.push(currentScore);
        LOG_INFO("[FrameScorer] EMA (alpha=" + std::to_string(config_.smoothingEMAAlpha) +
                 "): " + std::to_string(lastSmoothedScore_));
        return lastSmoothedScore_;
    }

    // SMA (Simple Moving Average) - fallback
    if (config_.smoothingWindowSize <= 1) {
        return currentScore;
    }

    scoreHistory_.push(currentScore);
    sumScores_ += currentScore;

    if (static_cast<int>(scoreHistory_.size()) > config_.smoothingWindowSize) {
        sumScores_ -= scoreHistory_.front();
        scoreHistory_.pop();
    }

    float smoothedScore = sumScores_ / scoreHistory_.size();
    LOG_INFO("[FrameScorer] SMA (window=" + std::to_string(config_.smoothingWindowSize) +
             "): " + std::to_string(smoothedScore));
    return smoothedScore;
}

}  // namespace KeyFrame
