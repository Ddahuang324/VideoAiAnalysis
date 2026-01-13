#include "TextDetector.h"

#include <algorithm>
#include <memory>
#include <mutex>
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
    std::lock_guard<std::mutex> lock(mutex_);
    const cv::Mat& frame = resource->getOriginalFrame();
    if (frame.empty()) {
        return Result();
    }

    std::vector<std::vector<cv::Point>> polygons = detectTextRegions(frame);

    if (polygons.empty()) {
        Result emptyResult;
        emptyResult.score = 0.0f;
        previousRegions_.clear();
        return emptyResult;
    }

    std::vector<TextRegion> currentRegions = processTextRegions(frame, polygons);

    Result result;
    result.textRegions = currentRegions;
    result.coverageRatio = computeCoverageRatio(currentRegions, frame.size());
    result.changeRatio = computeChangeRatio(currentRegions, previousRegions_);
    result.score = config_.alpha * result.coverageRatio + config_.beta * result.changeRatio;

    previousRegions_ = currentRegions;
    return result;
}

void TextDetector::reset() {
    std::lock_guard<std::mutex> lock(mutex_);
    previousRegions_.clear();
}

std::vector<std::vector<cv::Point>> TextDetector::detectTextRegions(const cv::Mat& frame) {
    std::vector<std::vector<cv::Point>> polygons;

    DataConverter::LetterboxInfo info;
    std::vector<float> inputData = DataConverter::matToTensorLetterbox(
        frame, cv::Size(config_.detInputWidth, config_.detInputHeight), info, true,
        {0.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f});

    std::vector<int64_t> inputShape = {1, 3, config_.detInputHeight, config_.detInputWidth};
    auto outputs =
        modelManager_.runInference("ch_PP-OCRv4_det_infer.onnx", {inputData}, {inputShape});
    if (outputs.empty() || outputs[0].empty()) {
        return polygons;
    }

    int outH = config_.detInputHeight;
    int outW = config_.detInputWidth;

    LOG_DEBUG(formatLog("[TextDetector] Det output size: ", outputs[0].size(),
                        ", Expected: ", outH * outW));

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
        if (area < 10) {
            continue;
        }

        std::vector<cv::Point> poly = mapContourToOriginal(contour, info);
        polygons.push_back(poly);
    }

    return polygons;
}

std::string TextDetector::recognizeText(const cv::Mat& textRegion) {
    int targetH = config_.recInputHeight;
    int targetW = config_.recInputWidth;

    cv::Mat resized;
    float aspect = static_cast<float>(textRegion.cols) / textRegion.rows;
    int newW = std::min(static_cast<int>(targetH * aspect), targetW);

    cv::resize(textRegion, resized, cv::Size(newW, targetH));

    cv::Mat padded = cv::Mat::zeros(targetH, targetW, textRegion.type());
    resized.copyTo(padded(cv::Rect(0, 0, newW, targetH)));

    std::vector<float> inputData = DataConverter::matToTensor(
        padded, cv::Size(targetW, targetH), true, {0.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f});

    std::vector<int64_t> inputShape = {1, 3, targetH, targetW};
    auto outputs =
        modelManager_.runInference("ch_PP-OCRv4_rec_infer.onnx", {inputData}, {inputShape});
    if (outputs.empty() || outputs[0].empty()) {
        return "";
    }

    // TODO: 实现完整的 CTC 解码和字典映射
    return "[Text]";
}

float TextDetector::computeCoverageRatio(const std::vector<TextRegion>& textRegions,
                                         const cv::Size& frameSize) {
    if (textRegions.empty()) {
        return 0.0f;
    }

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
    if (previousRegions.empty()) {
        return currentRegions.empty() ? 0.0f : 1.0f;
    }
    if (currentRegions.empty()) {
        return 1.0f;
    }

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

std::vector<TextDetector::TextRegion> TextDetector::processTextRegions(
    const cv::Mat& frame, const std::vector<std::vector<cv::Point>>& polygons) {
    std::vector<TextRegion> currentRegions;

    for (const auto& poly : polygons) {
        cv::Rect rect = cv::boundingRect(poly);
        rect &= cv::Rect(0, 0, frame.cols, frame.rows);
        if (rect.width < 4 || rect.height < 4) {
            continue;
        }

        TextRegion region;
        region.polygon = poly;
        region.boundingBox = rect;
        region.confidence = 1.0f;

        if (config_.enableRecognition) {
            cv::Mat crop = frame(rect);
            region.text = recognizeText(crop);
        } else {
            region.text = "";
        }

        currentRegions.push_back(region);
    }

    return currentRegions;
}

std::vector<cv::Point> TextDetector::mapContourToOriginal(
    const std::vector<cv::Point>& contour, const DataConverter::LetterboxInfo& info) {
    std::vector<cv::Point> poly;
    for (const auto& pt : contour) {
        float x = (pt.x - info.padLeft) / info.scale;
        float y = (pt.y - info.padTop) / info.scale;
        poly.push_back(cv::Point(static_cast<int>(x), static_cast<int>(y)));
    }
    return poly;
}

std::string TextDetector::formatLog(const std::string& prefix1, size_t value1,
                                    const std::string& prefix2, int value2) {
    std::ostringstream oss;
    oss << prefix1 << value1 << prefix2 << value2;
    return oss.str();
}

}  // namespace KeyFrame
