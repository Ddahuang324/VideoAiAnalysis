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

FrameScorer::FrameScorer(std::shared_ptr<DynamicCalculator> weightCalculator, const Config& config)
    : weightCalculator_(std::move(weightCalculator)), config_(config) {}

float FrameScorer::FuseScores(const MultiDimensionScore& scores,
                              std::vector<float>& appliedWeights) {
    // 获取当前权重
    if (config_.enableDynamicWeighting && weightCalculator_) {
        weightCalculator_->update(scores);
        appliedWeights = weightCalculator_->getCurrentWeights();
    } else {
        appliedWeights = {0.45f, 0.2f, 0.35f};  // 默认权重
    }

    float finalScore = scores.sceneScore * appliedWeights[0] +
                       scores.motionScore * appliedWeights[1] +
                       scores.textScore * appliedWeights[2];
    LOG_INFO("Final Score :" + std::to_string(finalScore));
    return finalScore;
}

FrameScore FrameScorer::score(const MultiDimensionScore& scores, const AnalysisContext& context) {
    std::lock_guard<std::mutex> lock(mutex_);
    FrameScore result;
    result.frameIndex = context.frameIndex;
    result.timestamp = context.timestamp;
    result.rawScores = scores;

    float fused = FuseScores(scores, result.appliedWeights);
    float boosteded = ApplyBoosts(fused, scores);

    result.finalscore = ApplyTemporalSmoothing(context.frameIndex, boosteded);

    // 记录各维度的贡献度（用于调试）
    result.sceneContribution = scores.sceneScore * result.appliedWeights[0];
    result.motionContribution = scores.motionScore * result.appliedWeights[1];
    result.textContribution = scores.textScore * result.appliedWeights[2];

    return result;
}

std::vector<FrameScore> FrameScorer::scoreBatch(const std::vector<MultiDimensionScore>& scoresBatch,
                                                const std::vector<AnalysisContext>& contexts) {
    if (scoresBatch.size() != contexts.size()) {
        LOG_ERROR("Scores batch size does not match contexts size.");
        return {};
    }

    std::vector<FrameScore> results;
    results.reserve(scoresBatch.size());

    for (size_t i = 0; i < scoresBatch.size(); ++i) {
        results.push_back(score(scoresBatch[i], contexts[i]));
    }

    return results;
}

void FrameScorer::reset() {
    std::lock_guard<std::mutex> lock(mutex_);
    while (!scoreHistory_.empty())
        scoreHistory_.pop();
    sumScores_ = 0.0f;  // 重置累积和
    lastSmoothedScore_ = 0.0f;
    if (weightCalculator_)
        weightCalculator_->reset();
}

float FrameScorer::ApplyBoosts(float baseScore, const MultiDimensionScore& scores) {
    float boostedScore = baseScore;

    if (config_.sceneChangeBoost > 1.0f && scores.sceneChangeResult.isSceneChange) {
        boostedScore *= config_.sceneChangeBoost;
        LOG_INFO("Applied Scene Change Boost: " + std::to_string(config_.sceneChangeBoost));
    }

    if (config_.motionIncreaseBoost > 1.0f && scores.motionResult.score > 0.5f) {
        boostedScore *= config_.motionIncreaseBoost;
        LOG_INFO("Applied Motion Increase Boost: " + std::to_string(config_.motionIncreaseBoost));
    }

    if (config_.textIncreaseBoost > 1.0f && scores.textResult.changeRatio > 0.1f) {
        boostedScore *= config_.textIncreaseBoost;
        LOG_INFO("Applied Text Increase Boost: " + std::to_string(config_.textIncreaseBoost));
    }

    return std::min(1.0f, boostedScore);
}

float FrameScorer::ApplyTemporalSmoothing(int frameIndex, float currentScore) {
    if (!config_.enbaleSmoothing) {
        return currentScore;
    }

    // 优先使用 EMA (指数移动平均)，响应更快，区分度更高
    if (config_.smoothingEMAAlpha > 0.0f && config_.smoothingEMAAlpha <= 1.0f) {
        if (scoreHistory_.empty()) {
            lastSmoothedScore_ = currentScore;
        } else {
            // EMA 公式: S_t = α * X_t + (1 - α) * S_{t-1}
            lastSmoothedScore_ = config_.smoothingEMAAlpha * currentScore +
                                 (1.0f - config_.smoothingEMAAlpha) * lastSmoothedScore_;
        }
        scoreHistory_.push(currentScore);  // 仅用于判断是否为第一帧
        LOG_INFO("Applied EMA Smoothing (alpha=" + std::to_string(config_.smoothingEMAAlpha) +
                 "): " + std::to_string(lastSmoothedScore_));
        return lastSmoothedScore_;
    }

    // 回退到 SMA (简单移动平均)
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
    LOG_INFO("Applied SMA Smoothing (window=" + std::to_string(config_.smoothingWindowSize) +
             "): " + std::to_string(smoothedScore));
    return smoothedScore;
}
}  // namespace KeyFrame
