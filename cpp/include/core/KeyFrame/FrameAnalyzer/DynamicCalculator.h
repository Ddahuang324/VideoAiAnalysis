#pragma once

#include <deque>
#include <vector>

#include "core/Config/UnifiedConfig.h"
#include "IFrameAnalyzer.h"

namespace KeyFrame {

// 使用统一配置系统的类型别名
using DynamicCalculatorConfig = Config::DynamicCalculatorConfig;

class DynamicCalculator {
public:
    // 使用统一配置
    using Config = DynamicCalculatorConfig;

    struct ActivationStats {
        std::vector<float> activations;     // 各个维度的激活度
        std::vector<float> dynamicWeights;  // 动态权重（归一化）
        std::vector<float> histroyAvg;      // 历史平均激活度
    };

    explicit DynamicCalculator(const Config& config);

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
