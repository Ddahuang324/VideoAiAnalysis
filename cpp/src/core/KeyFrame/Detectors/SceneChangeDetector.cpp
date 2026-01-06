#include "SceneChangeDetector.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <memory>
#include <opencv2/core/mat.hpp>
#include <opencv2/core/types.hpp>
#include <string>
#include <vector>

#include "DataConverter.h"
#include "FrameResource.h"
#include "Log.h"
#include "ModelManager.h"

namespace KeyFrame {

SceneChangeDetector::SceneChangeDetector(ModelManager& modelManager, const Config& config)
    : modelManager_(modelManager), config_(config), modelName_("MobileNet-v3-Small") {
    // 可选：预分配缓存空间
    if (config_.enableCache) {
        featureCache_.clear();
    }
}

SceneChangeDetector::Result SceneChangeDetector::detect(const cv::Mat& frame) {
    return detect(std::make_shared<FrameResource>(frame));
}

SceneChangeDetector::Result SceneChangeDetector::detect(std::shared_ptr<FrameResource> resource) {
    Result result;
    std::vector<float> inputData;

    // 使用 FrameResource 缓存预处理后的 Tensor
    std::string cacheKey = "scene_tensor_" + std::to_string(config_.inputsize);
    auto cachedTensor = resource->getOrGenerate<std::vector<float>>(cacheKey, [&]() {
        return std::make_shared<std::vector<float>>(preProcessFrame(resource->getOriginalFrame()));
    });

    if (!cachedTensor || cachedTensor->empty()) {
        LOG_ERROR("Failed to preprocess frame for scene change detection.");
        return result;
    }

    inputData = *cachedTensor;

    // 提取特征向量
    std::vector<float> currentFeature = extractFeature(inputData);
    if (currentFeature.empty()) {
        LOG_ERROR("Failed to extract feature for scene change detection.");
        return result;
    }

    result.currentFeature = currentFeature;

    // 计算相似度
    if (!featureCache_.empty()) {
        float similarity = computeCosineSimilarity(featureCache_.back(), currentFeature);
        result.similarity = similarity;

        // 分数归一化：将压缩的余弦距离映射到 [0, 1] 区间
        // MobileNet 分类向量的余弦相似度通常在 [0.6, 1.0] 范围内
        // 我们将这个范围归一化到 [0, 1]，使分数在多维加权时更有区分度
        constexpr float minSimilarity = 0.6f;   // 经验值：完全不同的场景
        constexpr float maxSimilarity = 0.98f;  // 经验值：几乎相同的帧

        // 线性归一化: (max - similarity) / (max - min)
        float rawScore = (maxSimilarity - similarity) / (maxSimilarity - minSimilarity);
        result.score = std::clamp(rawScore, 0.0f, 1.0f);  // 限制在 [0, 1] 范围

        result.isSceneChange = (similarity < config_.similarityThreshold);
    } else {
        result.similarity = 1.0f;
        result.score = 0.0f;
        result.isSceneChange = false;
    }

    // 更新缓存
    if (config_.enableCache) {
        featureCache_.push_back(currentFeature);
        if (featureCache_.size() > 5) {  // 限制缓存大小
            featureCache_.pop_front();
        }
    }

    return result;
}

void SceneChangeDetector::reset() {
    featureCache_.clear();
}

std::vector<float> SceneChangeDetector::preProcessFrame(const cv::Mat& frame) {
    if (frame.empty()) {
        return {};
    }

    std::vector<float> hwcData =
        DataConverter::matToTensor(frame, cv::Size(config_.inputsize, config_.inputsize), true,
                                   {0.485f, 0.456f, 0.406f}, {0.229f, 0.224f, 0.225f});

    // 2. 将 HWC 转换为 NCHW 格式
    return DataConverter::hwcToNchw(hwcData, config_.inputsize, config_.inputsize, 3);
}

std::vector<float> SceneChangeDetector::extractFeature(const std::vector<float>& inputData) {
    if (inputData.empty()) {
        return {};
    }

    // 通过 ModelManager 统一执行推理，
    std::vector<std::vector<float>> inputs = {inputData};
    auto outputs = modelManager_.runInference(modelName_, inputs);

    if (outputs.empty() || outputs[0].empty()) {
        return {};
    }

    return outputs[0];
}

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
}  // namespace KeyFrame