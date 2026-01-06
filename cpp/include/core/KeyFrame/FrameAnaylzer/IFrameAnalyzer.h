#pragma once

#include <memory>
#include <opencv2/core/mat.hpp>
#include <opencv2/core/types.hpp>
#include <string>

#include "FrameResource.h"
#include "MotionDetector.h"
#include "SceneChangeDetector.h"
#include "TextDetector.h"

namespace KeyFrame {

/**
 *@brief: 分析上下文，保存当前帧的相关信息和累计统计数据
 */

struct AnalysisContext {
    int frameIndex;      // 当前帧索引
    double timestamp;    // 当前帧时间戳（秒）
    cv::Size frameSize;  // 当前帧尺寸

    float avgSceneScore = 0.0f;   // 平均场景重要性分数
    float avgMotionScore = 0.0f;  // 平均运动重要性分数
    float avgTextScore = 0.0f;    // 平均文本重要性分数

    int totalFramesAnalyzed = 0;  // 分析的总帧数
};

struct MultiDimensionScore {
    float sceneScore = 0.0f;   // 场景重要性分数
    float motionScore = 0.0f;  // 运动重要性分数
    float textScore = 0.0f;    // 文本重要性分数

    SceneChangeDetector::Result sceneChangeResult;  // 场景变化检测结果
    MotionDetector::Result motionResult;            // 运动检测结果
    TextDetector::Result textResult;                // 文本检测结果
};

class IFrameAnalyzer {
public:
    virtual ~IFrameAnalyzer() = default;

    /**
     *@brief: 分析输入帧资源，返回多维度的重要性评分
     *@param resource: 帧资源，包含原始图像和可能的预处理缓存
     *@param context: 分析上下文，包含帧索引、时间戳等信息
     *@return: 多维度重要性评分结果
     */
    virtual MultiDimensionScore analyzeFrame(std::shared_ptr<FrameResource> resource,
                                             const AnalysisContext& context) = 0;

    // 获取分析器权重
    virtual float getBaseWeight() const = 0;

    // 获取分析器的名称
    virtual std::string getName() const = 0;

    // 重置分析器状态
    virtual void reset() = 0;
};
}  // namespace KeyFrame