#pragma once

#include <opencv2/core/types.hpp>

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
}  // namespace KeyFrame