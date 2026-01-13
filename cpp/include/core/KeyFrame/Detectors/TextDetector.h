#pragma once

#include <memory>
#include <mutex>
#include <opencv2/core/mat.hpp>
#include <opencv2/core/types.hpp>
#include <opencv2/opencv.hpp>
#include <string>
#include <vector>

#include "DataConverter.h"
#include "ModelManager.h"

namespace KeyFrame {

class FrameResource;  // 前向声明

class TextDetector {
public:
    struct Config {
        int detInputHeight = 960;  // 检测模型输入高度
        int detInputWidth = 960;   // 检测模型输入宽度
        int recInputHeight = 48;   // 识别模型输入高度
        int recInputWidth = 320;   // 识别模型输入宽度

        float detThreshold = 0.3f;  // 检测置信度阈值
        float recThreshold = 0.5f;  // 识别置信度阈值

        bool enableRecognition = false;  // 是否启用文本识别（识别很慢，评分阶段建议关闭）

        // 权重参数
        float alpha = 0.6f;  // 文本区域覆盖率权重
        float beta = 0.4f;   // 文本变化率权重
    };

    struct TextRegion {
        std::vector<cv::Point> polygon;  // 文本区域多边形
        std::string text;                // 识别结果文本
        float confidence;                // 识别置信度
        cv::Rect boundingBox;            // 包围盒
    };

    struct Result {
        std::vector<TextRegion> textRegions;  // 检测到的文本区域

        float score;  // 综合评分

        float coverageRatio;  // 文本覆盖率

        float changeRatio;  // 文本变化率
    };

    TextDetector(ModelManager& modelManager);
    TextDetector(ModelManager& modelManager, const Config& config);

    Result detect(const cv::Mat& frame);
    Result detect(std::shared_ptr<FrameResource> resource);

    void reset();

private:
    std::vector<std::vector<cv::Point>> detectTextRegions(const cv::Mat& frame);
    std::string recognizeText(const cv::Mat& textRegion);
    float computeCoverageRatio(const std::vector<TextRegion>& textRegions,
                               const cv::Size& frameSize);
    float computeChangeRatio(const std::vector<TextRegion>& currentRegions,
                             const std::vector<TextRegion>& previousRegions);
    std::vector<TextRegion> processTextRegions(const cv::Mat& frame,
                                               const std::vector<std::vector<cv::Point>>& polygons);
    std::vector<cv::Point> mapContourToOriginal(const std::vector<cv::Point>& contour,
                                                const DataConverter::LetterboxInfo& info);
    std::string formatLog(const std::string& prefix1, size_t value1, const std::string& prefix2,
                          int value2);

    std::vector<TextRegion> previousRegions_;
    ModelManager& modelManager_;
    Config config_;
    std::mutex mutex_;

};  // class TextDetector

}  // namespace KeyFrame