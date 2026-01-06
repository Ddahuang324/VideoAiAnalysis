
#include "TextDetector.h"

#include <opencv2/core/hal/interface.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <opencv2/core.hpp>
#include <opencv2/core/mat.hpp>
#include <opencv2/core/types.hpp>
#include <opencv2/imgproc.hpp>
#include <sstream>
#include <string>
#include <vector>

#include "DataConverter.h"
#include "FrameResource.h"
#include "Log.h"
#include "ModelManager.h"

namespace KeyFrame {

TextDetector::TextDetector(ModelManager& modelManager) : TextDetector(modelManager, Config()) {}

TextDetector::TextDetector(ModelManager& modelManager, const Config& config)
    : modelManager_(modelManager), config_(config) {
    // 确保模型已加载
    if (!modelManager_.hasModel("ch_PP-OCRv4_det_infer.onnx")) {
        LOG_WARN("Text detection model not loaded in ModelManager");
    }
    if (!modelManager_.hasModel("ch_PP-OCRv4_rec_infer.onnx")) {
        LOG_WARN("Text recognition model not loaded in ModelManager");
    }
}

TextDetector::Result TextDetector::detect(const cv::Mat& frame) {
    return detect(std::make_shared<FrameResource>(frame));
}

TextDetector::Result TextDetector::detect(std::shared_ptr<FrameResource> resource) {
    const cv::Mat& frame = resource->getOriginalFrame();
    if (frame.empty()) {
        return Result();
    }

    // 1. 检测文本区域 (PaddleOCR Det)
    // 注意：这里 detectTextRegions 内部也可以优化，但目前先保持原样，
    // 因为它涉及复杂的 Letterbox 预处理。
    std::vector<std::vector<cv::Point>> polygons = detectTextRegions(frame);

    // 流程图：检测到文本？ 否 -> 返回分数 0.0
    if (polygons.empty()) {
        Result emptyResult;
        emptyResult.score = 0.0f;
        previousRegions_.clear();
        return emptyResult;
    }

    // 2. 对每个区域进行识别 (PaddleOCR Rec)
    std::vector<TextRegion> currentRegions;
    for (const auto& poly : polygons) {
        cv::Rect rect = cv::boundingRect(poly);
        // 边界检查
        rect &= cv::Rect(0, 0, frame.cols, frame.rows);
        if (rect.width < 4 || rect.height < 4)
            continue;

        cv::Mat crop = frame(rect);
        std::string text = recognizeText(crop);

        TextRegion region;
        region.polygon = poly;
        region.text = text;
        region.confidence = 1.0f;  // 简化处理
        region.boundingBox = rect;
        currentRegions.push_back(region);
    }

    // 3. 计算指标与评分
    Result result;
    result.textRegions = currentRegions;
    result.coverageRatio = computeCoverageRatio(currentRegions, frame.size());

    // 流程图：文本缓存与前一帧对比
    result.changeRatio = computeChangeRatio(currentRegions, previousRegions_);

    // 流程图：重要性评分
    // 公式：文本重要性分数 = α × 文本区域覆盖率 + β × 文本变化率
    result.score = (config_.alpha * result.coverageRatio) + (config_.beta * result.changeRatio);

    previousRegions_ = currentRegions;
    return result;
}

void TextDetector::reset() {
    previousRegions_.clear();
}

std::vector<std::vector<cv::Point>> TextDetector::detectTextRegions(const cv::Mat& frame) {
    std::vector<std::vector<cv::Point>> polygons;

    // 预处理
    DataConverter::LetterboxInfo info;
    // PP-OCR 通常只进行归一化到 [0, 1]，不使用 ImageNet 的均值和标准差
    std::vector<float> inputData = DataConverter::matToTensorLetterbox(
        frame, cv::Size(config_.detInputWidth, config_.detInputHeight), info, true,
        {0.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f});

    // 推理
    std::vector<int64_t> inputShape = {1, 3, config_.detInputHeight, config_.detInputWidth};
    auto outputs =
        modelManager_.runInference("ch_PP-OCRv4_det_infer.onnx", {inputData}, {inputShape});
    if (outputs.empty() || outputs[0].empty()) {
        return polygons;
    }

    // 后处理 (DBNet)
    // 输出通常是 [1, 1, H, W] 的概率图
    int outH = config_.detInputHeight;
    int outW = config_.detInputWidth;

    {
        std::ostringstream oss;
        oss << "[TextDetector] Det output size: " << outputs[0].size()
            << ", Expected: " << (outH * outW);
        LOG_DEBUG(oss.str());
    }

    // 检查输出大小是否匹配
    if (outputs[0].size() != static_cast<size_t>(outH * outW)) {
        LOG_ERROR("Detection model output size mismatch");
        return polygons;
    }

    cv::Mat pred(outH, outW, CV_32F, const_cast<float*>(outputs[0].data()));

    cv::Mat bitMap;
    cv::threshold(pred, bitMap, config_.detThreshold, 255, cv::THRESH_BINARY);
    bitMap.convertTo(bitMap, CV_8U);

    std::vector<std::vector<cv::Point>> contours;
    cv::findContours(bitMap, contours, cv::RETR_LIST, cv::CHAIN_APPROX_SIMPLE);

    for (const auto& contour : contours) {
        float area = cv::contourArea(contour);
        if (area < 10)
            continue;

        // 将坐标映射回原图
        std::vector<cv::Point> poly;
        for (const auto& pt : contour) {
            float x = (pt.x - info.padLeft) / info.scale;
            float y = (pt.y - info.padTop) / info.scale;
            poly.push_back(cv::Point(static_cast<int>(x), static_cast<int>(y)));
        }
        polygons.push_back(poly);
    }

    return polygons;
}

std::string TextDetector::recognizeText(const cv::Mat& textRegion) {
    // 预处理：Resize 到高度 48，宽度按比例缩放或固定
    int targetH = config_.recInputHeight;
    int targetW = config_.recInputWidth;

    cv::Mat resized;
    float aspect = static_cast<float>(textRegion.cols) / textRegion.rows;
    int newW = static_cast<int>(targetH * aspect);
    newW = std::min(newW, targetW);

    cv::resize(textRegion, resized, cv::Size(newW, targetH));

    // 填充到 targetW
    cv::Mat padded = cv::Mat::zeros(targetH, targetW, textRegion.type());
    resized.copyTo(padded(cv::Rect(0, 0, newW, targetH)));

    // PP-OCR 识别模型通常也只进行归一化
    std::vector<float> inputData = DataConverter::matToTensor(
        padded, cv::Size(targetW, targetH), true, {0.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f});

    // 推理
    std::vector<int64_t> inputShape = {1, 3, targetH, targetW};
    auto outputs =
        modelManager_.runInference("ch_PP-OCRv4_rec_infer.onnx", {inputData}, {inputShape});
    if (outputs.empty() || outputs[0].empty()) {
        return "";
    }

    // 后处理 (CTC Greedy Search)
    // TODO: 实现完整的 CTC 解码和字典映射
    // 由于目前没有字典文件，暂时返回占位符
    return "[Text]";
}

float TextDetector::computeCoverageRatio(const std::vector<TextRegion>& textRegions,
                                         const cv::Size& frameSize) {
    if (textRegions.empty())
        return 0.0f;

    cv::Mat mask = cv::Mat::zeros(frameSize, CV_8U);
    for (const auto& region : textRegions) {
        std::vector<std::vector<cv::Point>> pts = {region.polygon};
        cv::fillPoly(mask, pts, cv::Scalar(255));
    }

    float whitePixels = cv::countNonZero(mask);
    return whitePixels / (frameSize.width * frameSize.height);
}

float TextDetector::computeChangeRatio(const std::vector<TextRegion>& currentRegions,
                                       const std::vector<TextRegion>& previousRegions) {
    if (previousRegions.empty())
        return currentRegions.empty() ? 0.0f : 1.0f;
    if (currentRegions.empty())
        return 1.0f;

    int matches = 0;
    for (const auto& curr : currentRegions) {
        for (const auto& prev : previousRegions) {
            cv::Rect intersect = curr.boundingBox & prev.boundingBox;
            float iou = static_cast<float>(intersect.area()) /
                        (curr.boundingBox.area() + prev.boundingBox.area() - intersect.area());

            if (iou > 0.5f) {
                matches++;
                break;
            }
        }
    }

    float matchRatio =
        static_cast<float>(matches) / std::max(currentRegions.size(), previousRegions.size());
    return 1.0f - matchRatio;
}

}  // namespace KeyFrame
