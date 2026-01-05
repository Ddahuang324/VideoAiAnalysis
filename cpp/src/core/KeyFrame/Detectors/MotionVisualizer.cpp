#include "MotionVisualizer.h"

#include <opencv2/core/hal/interface.h>

#include <algorithm>
#include <cstddef>
#include <iomanip>
#include <ios>
#include <opencv2/core.hpp>
#include <opencv2/core/mat.hpp>
#include <opencv2/core/matx.hpp>
#include <opencv2/core/types.hpp>
#include <opencv2/imgproc.hpp>
#include <set>
#include <sstream>
#include <string>
#include <vector>

#include "MotionDetector.h"

namespace KeyFrame {

// COCO 数据集 80 个类别
const char* MotionVisualizer::COCO_CLASSES[80] = {"person",        "bicycle",      "car",
                                                  "motorcycle",    "airplane",     "bus",
                                                  "train",         "truck",        "boat",
                                                  "traffic light", "fire hydrant", "stop sign",
                                                  "parking meter", "bench",        "bird",
                                                  "cat",           "dog",          "horse",
                                                  "sheep",         "cow",          "elephant",
                                                  "bear",          "zebra",        "giraffe",
                                                  "backpack",      "umbrella",     "handbag",
                                                  "tie",           "suitcase",     "frisbee",
                                                  "skis",          "snowboard",    "sports ball",
                                                  "kite",          "baseball bat", "baseball glove",
                                                  "skateboard",    "surfboard",    "tennis racket",
                                                  "bottle",        "wine glass",   "cup",
                                                  "fork",          "knife",        "spoon",
                                                  "bowl",          "banana",       "apple",
                                                  "sandwich",      "orange",       "broccoli",
                                                  "carrot",        "hot dog",      "pizza",
                                                  "donut",         "cake",         "chair",
                                                  "couch",         "potted plant", "bed",
                                                  "dining table",  "toilet",       "tv",
                                                  "laptop",        "mouse",        "remote",
                                                  "keyboard",      "cell phone",   "microwave",
                                                  "oven",          "toaster",      "sink",
                                                  "refrigerator",  "book",         "clock",
                                                  "vase",          "scissors",     "teddy bear",
                                                  "hair drier",    "toothbrush"};

MotionVisualizer::MotionVisualizer(const Config& config) : config_(config) {}

cv::Mat MotionVisualizer::draw(const cv::Mat& frame, const MotionDetector::Result& result,
                               int frameIndex) {
    if (frame.empty()) {
        return frame.clone();
    }

    cv::Mat canvas = frame.clone();

    // 更新轨迹历史
    if (config_.showTrackHistory) {
        updateTrackHistory(result.track);
        drawTrackHistory(canvas);
    }

    // 绘制边界框
    if (config_.showBoundingBoxes) {
        drawBoundingBoxes(canvas, result.track);
    }

    // 绘制速度向量
    if (config_.showVelocityArrows) {
        drawVelocityVectors(canvas, result.track);
    }

    // 绘制 HUD
    if (config_.showHUD) {
        drawHUD(canvas, result, frameIndex);
    }

    return canvas;
}

void MotionVisualizer::reset() {
    trackHistory_.clear();
}

void MotionVisualizer::drawBoundingBoxes(cv::Mat& canvas,
                                         const std::vector<MotionDetector::Track>& tracks) {
    for (const auto& track : tracks) {
        cv::Scalar color = getColorByTrackId(track.trackId);

        // 绘制边框
        cv::rectangle(canvas, track.box, color, config_.borderThickness);

        // 构建标签文本
        std::ostringstream labelStream;
        if (config_.showTrackIds) {
            labelStream << "ID:" << track.trackId;
        }
        if (config_.showConfidence) {
            if (config_.showTrackIds)
                labelStream << " ";
            labelStream << std::fixed << std::setprecision(0) << (track.confidence * 100) << "%";
        }
        if (config_.showClassLabels) {
            if (config_.showTrackIds || config_.showConfidence)
                labelStream << " ";
            labelStream << getClassName(track.classId);
        }

        std::string label = labelStream.str();

        if (!label.empty()) {
            // 计算文本大小
            int baseline = 0;
            cv::Size textSize = cv::getTextSize(label, cv::FONT_HERSHEY_SIMPLEX, 0.5, 1, &baseline);

            // 绘制标签背景
            int labelY = std::max(track.box.y, textSize.height + 5);
            cv::rectangle(canvas, cv::Point(track.box.x, labelY - textSize.height - 5),
                          cv::Point(track.box.x + textSize.width, labelY), color, -1);

            // 绘制文本
            cv::putText(canvas, label, cv::Point(track.box.x, labelY - 5), cv::FONT_HERSHEY_SIMPLEX,
                        0.5, cv::Scalar(255, 255, 255), 1);
        }
    }
}

void MotionVisualizer::drawVelocityVectors(cv::Mat& canvas,
                                           const std::vector<MotionDetector::Track>& tracks) {
    for (const auto& track : tracks) {
        float speed = cv::norm(track.Velocity);
        if (speed < 1.0f) {
            continue;  // 跳过静止或微小运动
        }

        // 计算中心点
        cv::Point2f center(track.box.x + track.box.width / 2.0f,
                           track.box.y + track.box.height / 2.0f);

        // 计算箭头终点（速度向量放大）
        cv::Point2f endpoint = center + cv::Point2f(track.Velocity.x * config_.velocityScale,
                                                    track.Velocity.y * config_.velocityScale);

        // 绘制箭头（黄色）
        cv::arrowedLine(canvas, center, endpoint, cv::Scalar(0, 255, 255), 2, cv::LINE_AA, 0, 0.3);
    }
}

void MotionVisualizer::drawTrackHistory(cv::Mat& canvas) {
    for (const auto& [trackId, history] : trackHistory_) {
        if (history.size() < 2) {
            continue;
        }

        cv::Scalar color = getColorByTrackId(trackId);

        // 绘制轨迹线，带渐变效果
        for (size_t i = 1; i < history.size(); ++i) {
            // 透明度随时间衰减
            double alpha = static_cast<double>(i) / history.size();
            cv::Scalar lineColor(color[0] * alpha, color[1] * alpha, color[2] * alpha);

            cv::line(canvas, history[i - 1], history[i], lineColor, 2, cv::LINE_AA);
        }
    }
}

void MotionVisualizer::drawHUD(cv::Mat& canvas, const MotionDetector::Result& result,
                               int frameIndex) {
    // HUD 位置和尺寸
    const int hudX = 10;
    const int hudY = 10;
    const int hudWidth = 360;
    const int hudHeight = 180;

    // 创建半透明背景
    cv::Mat overlay = canvas.clone();
    cv::rectangle(overlay, cv::Point(hudX, hudY), cv::Point(hudX + hudWidth, hudY + hudHeight),
                  cv::Scalar(0, 0, 0), -1);
    cv::addWeighted(overlay, config_.hudOpacity, canvas, 1.0 - config_.hudOpacity, 0, canvas);

    // 绘制边框
    cv::rectangle(canvas, cv::Point(hudX, hudY), cv::Point(hudX + hudWidth, hudY + hudHeight),
                  cv::Scalar(0, 255, 0), 2);

    // 文本配置
    int textX = hudX + 15;
    int textY = hudY + 30;
    const int lineHeight = 25;
    const cv::Scalar textColor(0, 255, 0);
    const double fontScale = 0.6;
    const int thickness = 1;

    auto drawText = [&](const std::string& text) {
        cv::putText(canvas, text, cv::Point(textX, textY), cv::FONT_HERSHEY_SIMPLEX, fontScale,
                    textColor, thickness, cv::LINE_AA);
        textY += lineHeight;
    };

    // 绘制信息
    drawText("Frame: " + std::to_string(frameIndex));

    std::ostringstream oss;
    oss << "Motion Score: " << std::fixed << std::setprecision(3) << result.score;
    drawText(oss.str());

    drawText("Active Tracks: " + std::to_string(result.track.size()));
    drawText("New Tracks: " + std::to_string(result.newTracks));
    drawText("Lost Tracks: " + std::to_string(result.LostTracks));

    oss.str("");
    oss << "Pixel Motion: " << std::fixed << std::setprecision(3) << result.pixelMotionScore;
    drawText(oss.str());

    oss.str("");
    oss << "Avg Velocity: " << std::fixed << std::setprecision(2) << result.avgVelocity << " px/f";
    drawText(oss.str());
}

cv::Scalar MotionVisualizer::getColorByTrackId(int trackId) {
    // 使用 HSV 色彩空间生成唯一颜色
    const int hue = (trackId * 37) % 180;  // 37 是质数，确保颜色分布均匀
    cv::Mat hsvColor(1, 1, CV_8UC3, cv::Scalar(hue, 255, 255));
    cv::Mat bgrColor;
    cv::cvtColor(hsvColor, bgrColor, cv::COLOR_HSV2BGR);

    cv::Vec3b pixel = bgrColor.at<cv::Vec3b>(0, 0);
    return cv::Scalar(pixel[0], pixel[1], pixel[2]);
}

std::string MotionVisualizer::getClassName(int classId) {
    if (classId >= 0 && classId < 80) {
        return COCO_CLASSES[classId];
    }
    return "unknown";
}

void MotionVisualizer::updateTrackHistory(const std::vector<MotionDetector::Track>& tracks) {
    // 为每个活跃轨迹更新历史点
    std::set<int> activeTrackIds;

    for (const auto& track : tracks) {
        activeTrackIds.insert(track.trackId);

        // 计算中心点
        cv::Point center(track.box.x + track.box.width / 2, track.box.y + track.box.height / 2);

        // 添加到历史
        auto& history = trackHistory_[track.trackId];
        history.push_back(center);

        // 限制历史长度
        if (static_cast<int>(history.size()) > config_.historyLength) {
            history.pop_front();
        }
    }

    // 移除非活跃轨迹的历史
    for (auto it = trackHistory_.begin(); it != trackHistory_.end();) {
        if (activeTrackIds.find(it->first) == activeTrackIds.end()) {
            it = trackHistory_.erase(it);
        } else {
            ++it;
        }
    }
}

}  // namespace KeyFrame
