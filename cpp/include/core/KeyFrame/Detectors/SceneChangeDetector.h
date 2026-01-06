#pragma once

#include <cstddef>
#include <deque>
#include <memory>
#include <opencv2/core/mat.hpp>
#include <opencv2/opencv.hpp>
#include <string>
#include <vector>

#include "ModelManager.h"

namespace KeyFrame {

class FrameResource;  // 前向声明

class SceneChangeDetector {
public:
    struct Config {
        float similarityThreshold = 0.8f;  // 相似度阈值
        int featureDim = 1000;             // 特征维度 (匹配 MobileNet-v3-Small 输出)
        int inputsize = 224;               // 输入尺寸
        bool enableCache = true;           // 启用缓存以加速处理

        Config() {}
    };

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
    // 预处理帧
    std::vector<float> preProcessFrame(const cv::Mat& frame);

    // 提取特征向量
    std::vector<float> extractFeature(const std::vector<float>& inputData);

    // 计算余弦相似度
    float computeCosineSimilarity(const std::vector<float>& feat1, const std::vector<float>& feat2);

    ModelManager& modelManager_;
    Config config_;
    std::string modelName_;
    std::deque<std::vector<float>> featureCache_;

    static constexpr size_t MAX_CACHE_SIZE = 2;
};

}  // namespace KeyFrame