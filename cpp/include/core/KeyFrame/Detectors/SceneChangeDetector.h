#pragma once

#include <cstddef>
#include <deque>
#include <memory>
#include <mutex>
#include <opencv2/core/mat.hpp>
#include <opencv2/opencv.hpp>
#include <string>
#include <vector>

#include "core/Config/UnifiedConfig.h"
#include "ModelManager.h"

namespace KeyFrame {

class FrameResource;  // 前向声明

// 使用统一配置系统的类型别名
using SceneChangeDetectorConfig = Config::SceneChangeDetectorConfig;

class SceneChangeDetector {
public:
    // 使用统一配置
    using Config = SceneChangeDetectorConfig;

    struct Result {
        bool isSceneChange = false;         // 是否发生场景变化
        float score = 0.0f;                 // 场景变化分数
        float similarity = 1.0f;            // 帧间相似度
        std::vector<float> currentFeature;  // 当前帧特征向量
    };

    // 构造函数
    SceneChangeDetector(ModelManager& modelManager, const Config& config = Config());

    // 检测场景变化
    Result detect(const cv::Mat& frame);
    Result detect(std::shared_ptr<FrameResource> resource);

    // 重置状态
    void reset();

    // 获取当前配置
    const Config& getConfig() const { return config_; }

private:
    std::vector<float> preProcessFrame(const cv::Mat& frame);
    std::vector<float> extractFeature(const std::vector<float>& inputData);
    float computeCosineSimilarity(const std::vector<float>& feat1, const std::vector<float>& feat2);
    float normalizeScore(float similarity);

    ModelManager& modelManager_;
    Config config_;
    std::string modelName_;
    std::deque<std::vector<float>> featureCache_;
    std::mutex mutex_;

    static constexpr size_t MAX_CACHE_SIZE = 2;
};

}  // namespace KeyFrame
