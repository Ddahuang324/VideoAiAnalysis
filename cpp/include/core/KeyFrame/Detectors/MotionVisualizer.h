#pragma once

#include <deque>
#include <map>
#include <opencv2/core/mat.hpp>
#include <opencv2/core/types.hpp>
#include <string>
#include <vector>

#include "MotionDetector.h"

namespace KeyFrame {

/**
 * @brief 运动检测可视化器
 *
 * 为 MotionDetector 的检测结果提供可视化功能，包括：
 * - 边界框绘制
 * - Track ID 和置信度标签
 * - 速度向量箭头
 * - 轨迹历史轨迹
 * - HUD 信息面板
 */
class MotionVisualizer {
public:
    struct Config {
        bool showBoundingBoxes;   // 显示边界框
        bool showTrackIds;        // 显示 Track ID
        bool showConfidence;      // 显示置信度
        bool showVelocityArrows;  // 显示速度箭头
        bool showTrackHistory;    // 显示轨迹历史
        int historyLength;        // 轨迹历史长度（帧数）
        bool showHUD;             // 显示 HUD 信息面板
        bool showClassLabels;     // 显示 COCO 类别标签
        float hudOpacity;         // HUD 背景透明度 [0.0, 1.0]
        int borderThickness;      // 边框线条粗细
        float velocityScale;      // 速度向量缩放系数

        // 默认构造函数
        Config()
            : showBoundingBoxes(true),
              showTrackIds(true),
              showConfidence(true),
              showVelocityArrows(true),
              showTrackHistory(true),
              historyLength(30),
              showHUD(true),
              showClassLabels(true),
              hudOpacity(0.7f),
              borderThickness(2),
              velocityScale(5.0f) {}
    };

    /**
     * @brief 构造函数
     * @param config 可视化配置
     */
    explicit MotionVisualizer(const Config& config = Config());

    /**
     * @brief 绘制检测结果到帧上
     * @param frame 原始帧
     * @param result 检测结果
     * @param frameIndex 当前帧索引
     * @return 带标注的帧
     */
    cv::Mat draw(const cv::Mat& frame, const MotionDetector::Result& result, int frameIndex);

    /**
     * @brief 重置可视化器状态（清除轨迹历史）
     */
    void reset();

private:
    /**
     * @brief 绘制边界框和标签
     */
    void drawBoundingBoxes(cv::Mat& canvas, const std::vector<MotionDetector::Track>& tracks);

    /**
     * @brief 绘制速度向量箭头
     */
    void drawVelocityVectors(cv::Mat& canvas, const std::vector<MotionDetector::Track>& tracks);

    /**
     * @brief 绘制轨迹历史
     */
    void drawTrackHistory(cv::Mat& canvas);

    /**
     * @brief 绘制 HUD 信息面板
     */
    void drawHUD(cv::Mat& canvas, const MotionDetector::Result& result, int frameIndex);

    /**
     * @brief 根据 Track ID 获取唯一颜色
     * @param trackId Track ID
     * @return BGR 颜色
     */
    cv::Scalar getColorByTrackId(int trackId);

    /**
     * @brief 获取 COCO 类别名称
     * @param classId 类别 ID
     * @return 类别名称
     */
    std::string getClassName(int classId);

    /**
     * @brief 更新轨迹历史
     */
    void updateTrackHistory(const std::vector<MotionDetector::Track>& tracks);

    /**
     * @brief 构建轨迹标签文本
     */
    std::string buildLabel(const MotionDetector::Track& track);

    /**
     * @brief 绘制单个标签
     */
    void drawLabel(cv::Mat& canvas, const cv::Rect& box, const std::string& label,
                   const cv::Scalar& color);

    /**
     * @brief 获取边界框中心点
     */
    cv::Point getBoxCenter(const cv::Rect& box);

    /**
     * @brief 格式化浮点数文本
     */
    std::string formatFloat(const std::string& prefix, float value, int precision);

    Config config_;
    std::map<int, std::deque<cv::Point>> trackHistory_;

    static const char* COCO_CLASSES[80];
};

}  // namespace KeyFrame
