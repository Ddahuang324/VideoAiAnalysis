#include "SceneChangeDetector.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <memory>
#include <mutex>
#include <opencv2/core/mat.hpp>
#include <string>
#include <vector>

#include "DataConverter.h"
#include "FrameResource.h"
#include "Log.h"
#include "ModelManager.h"
#include "opencv2/core/types.hpp"

namespace KeyFrame {

// ========== Constructor ==========

SceneChangeDetector::SceneChangeDetector(ModelManager& modelManager, const Config& config)
    : modelManager_(modelManager), config_(config), modelName_("MobileNet-v3-Small") {
    if (config_.enableCache) {
        featureCache_.clear();
    }
}

// ========== Detection ==========

SceneChangeDetector::Result SceneChangeDetector::detect(const cv::Mat& frame) {
    return detect(std::make_shared<FrameResource>(frame));
}

SceneChangeDetector::Result SceneChangeDetector::detect(std::shared_ptr<FrameResource> resource) {
    std::lock_guard<std::mutex> lock(mutex_);
    Result result;

    std::string cacheKey = "scene_tensor_" + std::to_string(config_.inputSize);
    auto cachedTensor = resource->getOrGenerate<std::vector<float>>(cacheKey, [&]() {
        return std::make_shared<std::vector<float>>(preProcessFrame(resource->getOriginalFrame()));
    });

    if (!cachedTensor || cachedTensor->empty()) {
        LOG_ERROR("[SceneChangeDetector] Preprocessing failed");
        return result;
    }

    std::vector<float> currentFeature = extractFeature(*cachedTensor);
    if (currentFeature.empty()) {
        LOG_ERROR("[SceneChangeDetector] Feature extraction failed");
        return result;
    }

    result.currentFeature = currentFeature;

    if (!featureCache_.empty()) {
        float similarity = computeCosineSimilarity(featureCache_.back(), currentFeature);
        result.similarity = similarity;
        result.score = normalizeScore(similarity);
        result.isSceneChange = (similarity < config_.similarityThreshold);
    } else {
        // 第一帧：视为场景切换，因为它打破了之前的"无画面"状态
        result.similarity = 0.0f;     // 第一帧没有前序帧，相似度定义为 0
        result.score = 1.0f;          // 第一帧的分数应最高
        result.isSceneChange = true;  // 必须判断为场景切换，以确保第一帧被捕获
    }

    if (config_.enableCache) {
        featureCache_.push_back(currentFeature);
        if (featureCache_.size() > 5) {
            featureCache_.pop_front();
        }
    }

    // Log detection result
    LOG_INFO("[SceneChangeDetector] Score: " + std::to_string(result.score) +
             ", Similarity: " + std::to_string(result.similarity) +
             ", SceneChange: " + std::to_string(result.isSceneChange));

    return result;
}

void SceneChangeDetector::reset() {
    std::lock_guard<std::mutex> lock(mutex_);
    featureCache_.clear();
}

// ========== Preprocessing ==========

std::vector<float> SceneChangeDetector::preProcessFrame(const cv::Mat& frame) {
    if (frame.empty()) {
        return {};
    }

    // Convert to tensor (HWC format)
    std::vector<float> hwcData =
        DataConverter::matToTensor(frame, cv::Size(config_.inputSize, config_.inputSize), true,
                                   {0.485f, 0.456f, 0.406f}, {0.229f, 0.224f, 0.225f});

    // Convert HWC to NCHW
    return DataConverter::hwcToNchw(hwcData, config_.inputSize, config_.inputSize, 3);
}

// ========== Feature Extraction ==========

std::vector<float> SceneChangeDetector::extractFeature(const std::vector<float>& inputData) {
    if (inputData.empty()) {
        return {};
    }

    std::vector<std::vector<float>> inputs = {inputData};
    auto outputs = modelManager_.runInference(modelName_, inputs);

    if (outputs.empty() || outputs[0].empty()) {
        return {};
    }

    return outputs[0];
}

// ========== Similarity Computation ==========

float SceneChangeDetector::computeCosineSimilarity(const std::vector<float>& feat1,
                                                   const std::vector<float>& feat2) {
    if (feat1.size() != feat2.size() || feat1.empty()) {
        return 0.0f;
    }

    float dotProduct = 0.0f;
    float normA = 0.0f;
    float normB = 0.0f;

    for (size_t i = 0; i < feat1.size(); ++i) {
        dotProduct += feat1[i] * feat2[i];
        normA += feat1[i] * feat1[i];
        normB += feat2[i] * feat2[i];
    }

    if (normA == 0.0f || normB == 0.0f) {
        return 0.0f;
    }

    return dotProduct / (std::sqrt(normA) * std::sqrt(normB));
}

float SceneChangeDetector::normalizeScore(float similarity) {
    constexpr float MIN_SIMILARITY = 0.6f;
    constexpr float MAX_SIMILARITY = 0.98f;
    float rawScore = (MAX_SIMILARITY - similarity) / (MAX_SIMILARITY - MIN_SIMILARITY);
    return std::clamp(rawScore, 0.0f, 1.0f);
}

}  // namespace KeyFrame
