#pragma once

#include <memory>
#include <queue>
#include <vector>

#include "DynamicCalculator.h"
#include "IFrameAnalyzer.h"

namespace KeyFrame {

struct FrameScore {
    int frameIndex;    // 当前帧索引
    double timestamp;  // 当前帧时间戳
    float finalscore;  // 综合得分

    float sceneContribution;   // 场景贡献度
    float motionContribution;  // 运动贡献度
    float textContribution;    // 文本贡献度

    MultiDimensionScore rawScores;      // 各维度原始分数
    std::vector<float> appliedWeights;  // 各维度应用的权重
};

class FrameScorer {
public:
    struct Config {
        bool enableDynamicWeighting;  // 是否启用动态权重调整

        bool enbaleSmoothing;     // 是否启用时间平滑
        int smoothingWindowSize;  // 平滑窗口大小（帧数，用于 SMA）
        float smoothingEMAAlpha;  // EMA 平滑系数 (0.0 - 1.0, 1.0 表示无平滑)，默认为 0 表示使用 SMA

        float sceneChangeBoost;     // 场景变化时的分数提升因子
        float motionIncreaseBoost;  // 运动增加时的分数提升因子
        float textIncreaseBoost;    // 文本增加时的分数提升因子

        Config()
            : enableDynamicWeighting(true),
              enbaleSmoothing(true),
              smoothingWindowSize(3),
              smoothingEMAAlpha(0.6f),
              sceneChangeBoost(1.2f),
              motionIncreaseBoost(1.1f),
              textIncreaseBoost(1.1f) {}
    };

    explicit FrameScorer(std::shared_ptr<DynamicCalculator> weightCalculator,
                         const Config& config = Config());

    /**
     *@brief: 根据多维度评分和上下文信息计算最终帧分数
     *@param scores: 多维度评分结果
     *@param context: 分析上下文信息
     *@return: 计算得到的帧分数
     */
    FrameScore score(const MultiDimensionScore& scores, const AnalysisContext& context);

    /**
     *@brief: 批量计算帧分数，支持时间平滑
     *@param scoresBatch: 多维度评分结果批次
     *@param contexts: 分析上下文信息批次
     *@return: 计算得到的帧分数列表
     */
    std::vector<FrameScore> scoreBatch(const std::vector<MultiDimensionScore>& scoresBatch,
                                       const std::vector<AnalysisContext>& contexts);

    void reset();

private:
    /**
     *@brief: 融合多维度评分为最终分数
     *@param scores: 多维度评分结果
     *@param appliedWeights: 输出参数，返回各维度应用的权重
     */
    float FuseScores(const MultiDimensionScore& scores, std::vector<float>& appliedWeights);

    float ApplyTemporalSmoothing(int frameIndex, float rawScore);

    /**
     *@brief: 根据上下文信息应用提升因子
     *@param score: 原始分数
     *@param context: 分析上下文信息
     *@return: 应用提升因子后的分数
     */
    float ApplyBoosts(float score, const MultiDimensionScore& rawScores);

    std::shared_ptr<DynamicCalculator> weightCalculator_;
    Config config_;

    std::queue<float> scoreHistory_;
    float sumScores_ = 0.0f;  // 累积和，用于高效计算平均值
    float lastSmoothedScore_ = 0.0f;
};
}  // namespace KeyFrame