#pragma once

#include <deque>
#include <vector>

#include "IFrameAnalyzer.h"

namespace KeyFrame {

class DynamicCalculator {
public:
    struct Config {
        std::vector<float> baseWeights = {0.45f, 0.2f, 0.35f};  // 场景、运动、文本的基础权重

        float currentFrameWeight = 0.3f;   // 当前帧权重
        float activationInfluence = 0.5f;  // 激活度的重要性

        int historyWindowSize = 30;  // 历史帧数窗口大小

        float minWeight = 0.05f;  // 单个维度最小的权重
        float maxWeight = 0.7f;   // 单个维度最大的权重
    };

    struct ActivationStats {
        std::vector<float> activations;     // 各个维度的激活度
        std::vector<float> dynamicWeights;  // 动态权重（归一化）
        std::vector<float> histroyAvg;      // 历史平均激活度
    };

    explicit DynamicCalculator(const Config& config) : config_(config) {};

    ActivationStats update(const MultiDimensionScore& scores);

    const std::vector<float>& getCurrentWeights() const { return currentWeights_; }

    void reset();

    const Config& getConfig() const { return config_; }

private:
    std::vector<float> calculateActivations(const std::vector<float>& currentscores);

    std::vector<float> normaliseWeights(const std::vector<float>& rawWeights);

    Config config_;                      // 配置参数
    std::vector<float> currentWeights_;  // 当前权重

    std::deque<std::vector<float>> historyScores_;  // 历史分数记录 (改为 deque 以支持高效首尾操作)
    std::vector<float> historyAverages_;            // 历史平均分数
    std::vector<float> runningSum_;                 // 累加和，用于 O(1) 计算平均值
};

}  // namespace KeyFrame