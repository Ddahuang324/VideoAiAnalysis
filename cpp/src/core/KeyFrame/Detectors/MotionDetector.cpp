#include "MotionDetector.h"

#include <algorithm>
#include <iomanip>
#include <ios>
#include <memory>
#include <mutex>
#include <opencv2/core.hpp>
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

MotionDetector::MotionDetector(ModelManager& modelManager, const Config& config,
                               const std::string& modelName)
    : modelName_(modelName.empty() ? "yolov8n.onnx" : modelName),
      modelManager_(modelManager),
      config_(config) {}

MotionDetector::Result MotionDetector::detect(const cv::Mat& frame) {
    return detect(std::make_shared<FrameResource>(frame));
}

MotionDetector::Result MotionDetector::detect(std::shared_ptr<FrameResource> resource) {
    std::lock_guard<std::mutex> lock(mutex_);
    const cv::Mat& frame = resource->getOriginalFrame();

    if (frame.empty()) {
        LOG_WARN("[MotionDetector] 输入帧为空，返回默认结果");
        return Result{0.0f, {}, 0, 0, 0.0f, 0.0f};
    }

    // 预处理
    std::string cacheKey = "motion_tensor_" + std::to_string(config_.inputWidth);
    auto cachedTensor = resource->getOrGenerate<std::vector<float>>(
        cacheKey, [&]() { return std::make_shared<std::vector<float>>(preprocessFrame(frame)); });

    if (!cachedTensor || cachedTensor->empty()) {
        LOG_ERROR("[MotionDetector] 预处理失败，返回默认结果");
        return Result{0.0f, activeTracks_, 0, 0, 0.0f, 0.0f};
    }

    // 模型推理
    std::vector<std::vector<float>> outputs =
        modelManager_.runInference(modelName_, {*cachedTensor});
    if (outputs.empty() || outputs[0].empty()) {
        LOG_ERROR("[MotionDetector] 模型推理失败或输出为空");
        return Result{0.0f, activeTracks_, 0, 0, 0.0f, 0.0f};
    }
    LOG_DEBUG(formatLog("推理完成，输出大小: ", outputs[0].size()));

    // 后处理
    std::vector<Detection> detections = postprocessDetections(outputs, frame.size());
    LOG_DEBUG(formatLog("检测到 ", detections.size(), " 个目标"));

    // 更新跟踪器
    int newTracks = 0;
    int lostTracks = 0;
    UpdateTracks(detections, newTracks, lostTracks);
    LOG_DEBUG(formatLog("跟踪更新完成 - 活跃: ", activeTracks_.size(), ", 新增: ", newTracks,
                        ", 丢失: ", lostTracks));

    // 计算像素级运动和运动得分
    float pixelMotion = calculatePixelMotion(frame);
    float score = ComputeMotionScore(activeTracks_, newTracks, lostTracks, pixelMotion);

    // 计算平均速度
    float avgVelocity = calculateAverageVelocity();

    LOG_INFO(formatLogFixed("检测完成 - 得分: ", score, 3) +
             formatLogFixed(", 像素运动: ", pixelMotion, 3) +
             formatLogFixed(", 平均速度: ", avgVelocity, 2));

    return Result{score, activeTracks_, newTracks, lostTracks, avgVelocity, pixelMotion};
}

float MotionDetector::calculatePixelMotion(const cv::Mat& frame) {
    if (frame.empty()) {
        return 0.0f;
    }

    cv::Mat smallFrame;
    cv::resize(frame, smallFrame, cv::Size(640, 360));

    cv::Mat gray;
    if (smallFrame.channels() == 3) {
        cv::cvtColor(smallFrame, gray, cv::COLOR_BGR2GRAY);
    } else {
        gray = smallFrame.clone();
    }

    cv::GaussianBlur(gray, gray, cv::Size(5, 5), 0);

    if (prevGrayFrame_.empty() || prevGrayFrame_.size() != gray.size()) {
        prevGrayFrame_ = gray;
        return 0.0f;
    }

    cv::Mat diff;
    cv::absdiff(prevGrayFrame_, gray, diff);
    cv::threshold(diff, diff, 25, 255, cv::THRESH_BINARY);

    cv::Mat kernel = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(3, 3));
    cv::erode(diff, diff, kernel);
    cv::dilate(diff, diff, kernel);

    int totalPixels = diff.cols * diff.rows;
    int motionPixels = cv::countNonZero(diff);

    prevGrayFrame_ = gray;

    float rawScore = static_cast<float>(motionPixels) / totalPixels;
    return std::min(rawScore * 50.0f, 1.0f);
}

float MotionDetector::ComputeMotionScore(const std::vector<Track>& tracks, int newTracks,
                                         int lostTracks, float pixelMotion) {
    const float alpha = 0.3f;
    const float beta = 0.5f;
    const float gamma = 0.2f;

    float objectCountScore = std::min(static_cast<float>(tracks.size()) / 10.0f, 1.0f);
    float avgSpeed = calculateAverageVelocity();
    float speedScore = std::min(avgSpeed / 20.0f, 1.0f);
    float changeScore = std::min((newTracks + lostTracks) / 10.0f, 1.0f);

    float objectMotionScore = alpha * objectCountScore + beta * speedScore + gamma * changeScore;
    float finalScore =
        config_.pixelMotionWeight * pixelMotion + config_.objectMotionWeight * objectMotionScore;

    return std::min(finalScore, 1.0f);
}

void MotionDetector::reset() {
    std::lock_guard<std::mutex> lock(mutex_);
    LOG_INFO(formatLog("重置检测器，清除 ", activeTracks_.size(), " 个活跃轨迹"));
    activeTracks_.clear();
    lostTracks_.clear();
    trackLostFrames_.clear();
    nextTrackId_ = 0;
    prevGrayFrame_ = cv::Mat();
}

std::vector<float> MotionDetector::preprocessFrame(const cv::Mat& frame) {
    if (frame.empty()) {
        LOG_WARN("[MotionDetector] 预处理输入帧为空");
        return {};
    }

    try {
        auto result = DataConverter::matToTensorLetterbox(
            frame, cv::Size(config_.inputWidth, config_.inputWidth), letterboxInfo_, true,
            {0.485f, 0.456f, 0.406f}, {0.229f, 0.224f, 0.225f});

        if (result.empty()) {
            LOG_ERROR("[MotionDetector] Letterbox 转换返回空数据");
        }
        return result;
    } catch (const std::exception& e) {
        LOG_ERROR(formatLog("预处理异常: ", e.what()));
        return {};
    }
}

std::vector<MotionDetector::Detection> MotionDetector::postprocessDetections(
    const std::vector<std::vector<float>>& outputs, const cv::Size& originalSize) {
    if (outputs.empty() || outputs[0].empty()) {
        LOG_ERROR("[MotionDetector] 后处理输入为空");
        return {};
    }

    const int numClasses = 80;
    const int numProposals = 8400;
    const size_t expectedSize = static_cast<size_t>(numClasses + 4) * numProposals;

    if (outputs[0].size() < expectedSize) {
        LOG_ERROR(
            formatLog("模型输出大小不匹配. 期望: ", expectedSize, ", 实际: ", outputs[0].size()));
        return {};
    }

    std::vector<cv::Rect> bboxes;
    std::vector<float> scores;
    std::vector<int> classIds;

    const float* output = outputs[0].data();

    for (int i = 0; i < numProposals; ++i) {
        float maxScore = 0.0f;
        int maxClassId = 0;

        for (int c = 0; c < numClasses; ++c) {
            float score = output[(4 + c) * numProposals + i];
            if (score > maxScore) {
                maxScore = score;
                maxClassId = c;
            }
        }

        if (maxScore < config_.confidenceThreshold) {
            continue;
        }

        float cx = output[0 * numProposals + i];
        float cy = output[1 * numProposals + i];
        float w = output[2 * numProposals + i];
        float h = output[3 * numProposals + i];

        int x = static_cast<int>(cx - w / 2);
        int y = static_cast<int>(cy - h / 2);
        int width = static_cast<int>(w);
        int height = static_cast<int>(h);

        cv::Rect rect = DataConverter::rescaleBox(cv::Rect(x, y, width, height), letterboxInfo_);

        bboxes.push_back(rect);
        scores.push_back(maxScore);
        classIds.push_back(maxClassId);
    }

    std::vector<int> nmsIndices;
    cv::dnn::NMSBoxes(bboxes, scores, config_.confidenceThreshold, config_.nmsThreshold,
                      nmsIndices);

    std::vector<Detection> nmsResults;
    for (int idx : nmsIndices) {
        if (idx >= 0 && static_cast<size_t>(idx) < bboxes.size()) {
            nmsResults.push_back({bboxes[idx], scores[idx], classIds[idx]});
        } else {
            LOG_ERROR(formatLog("NMS 索引越界: ", idx, ", bboxes size: ", bboxes.size()));
        }
    }

    return nmsResults;
}

void MotionDetector::UpdateTracks(const std::vector<Detection>& detections, int& newTracks,
                                  int& lostTracks) {
    std::vector<Track> nextActiveTracks;
    std::vector<bool> detectionUsed(detections.size(), false);
    newTracks = 0;
    lostTracks = 0;

    // 1. 关联现有活跃轨迹
    for (auto& track : activeTracks_) {
        float maxIOU = 0.3f;  // 最小匹配阈值
        int bestIdx = -1;

        for (size_t i = 0; i < detections.size(); ++i) {
            if (detectionUsed[i])
                continue;
            float iou = calculateIOU(track.box, detections[i].box);
            if (iou > maxIOU) {
                maxIOU = iou;
                bestIdx = i;
            }
        }

        if (bestIdx != -1) {
            detectionUsed[bestIdx] = true;
            updateTrackInfo(track, detections[bestIdx]);
            nextActiveTracks.push_back(track);
            trackLostFrames_[track.trackId] = 0;
        } else {
            trackLostFrames_[track.trackId]++;
            lostTracks++;
            lostTracks_.push_back(track);
        }
    }

    // 2. 尝试从丢失轨迹中恢复
    auto it = lostTracks_.begin();
    while (it != lostTracks_.end()) {
        float maxIOU = 0.3f;
        int bestIdx = -1;

        for (size_t i = 0; i < detections.size(); ++i) {
            if (detectionUsed[i])
                continue;
            float iou = calculateIOU(it->box, detections[i].box);
            if (iou > maxIOU) {
                maxIOU = iou;
                bestIdx = i;
            }
        }

        if (bestIdx != -1) {
            detectionUsed[bestIdx] = true;
            updateTrackInfo(*it, detections[bestIdx]);
            nextActiveTracks.push_back(*it);
            trackLostFrames_[it->trackId] = 0;
            it = lostTracks_.erase(it);
        } else {
            trackLostFrames_[it->trackId]++;
            if (trackLostFrames_[it->trackId] >= config_.trackBufferSize) {
                trackLostFrames_.erase(it->trackId);
                it = lostTracks_.erase(it);
            } else {
                ++it;
            }
        }
    }

    // 3. 处理未匹配的检测框（创建新轨迹）
    for (size_t i = 0; i < detections.size(); ++i) {
        if (!detectionUsed[i] && detections[i].confidence > config_.trackHighThreshold) {
            Track newTrack;
            newTrack.trackId = nextTrackId_++;
            updateTrackInfo(newTrack, detections[i]);
            newTrack.Velocity = cv::Point2f(0, 0);

            nextActiveTracks.push_back(newTrack);
            trackLostFrames_[newTrack.trackId] = 0;
            newTracks++;
        }
    }

    activeTracks_ = nextActiveTracks;
}

void MotionDetector::updateTrackInfo(Track& track, const Detection& det) {
    cv::Point2f newCenter(det.box.x + det.box.width / 2.0f, det.box.y + det.box.height / 2.0f);

    if (track.box.area() > 0) {
        cv::Point2f oldCenter(track.box.x + track.box.width / 2.0f,
                              track.box.y + track.box.height / 2.0f);
        cv::Point2f newVelocity = newCenter - oldCenter;

        const float alpha = 0.7f;
        track.Velocity = alpha * newVelocity + (1.0f - alpha) * track.Velocity;

        float speed = cv::norm(track.Velocity);
        if (speed > 100.0f) {
            LOG_WARN(formatLogFixed("轨迹 ", track.trackId, " 速度异常: ", speed, 2));
        }
    } else {
        track.Velocity = cv::Point2f(0, 0);
        LOG_DEBUG(formatLog("初始化新轨迹 ", track.trackId));
    }

    track.box = det.box;
    track.confidence = det.confidence;
    track.classId = det.classId;
}

float MotionDetector::calculateIOU(const cv::Rect& box1, const cv::Rect& box2) {
    int interArea = (box1 & box2).area();
    int unionArea = box1.area() + box2.area() - interArea;
    if (unionArea <= 0) {
        return 0.0f;
    }
    return static_cast<float>(interArea) / unionArea;
}

float MotionDetector::calculateAverageVelocity() {
    float totalVelocity = 0.0f;
    for (const auto& track : activeTracks_) {
        totalVelocity += cv::norm(track.Velocity);
    }
    return activeTracks_.empty() ? 0.0f : totalVelocity / activeTracks_.size();
}

std::string MotionDetector::formatLog(const std::string& prefix, int value) {
    std::ostringstream oss;
    oss << "[MotionDetector] " << prefix << value;
    return oss.str();
}

std::string MotionDetector::formatLog(const std::string& prefix1, int value1,
                                      const std::string& prefix2, int value2) {
    std::ostringstream oss;
    oss << "[MotionDetector] " << prefix1 << value1 << prefix2 << value2;
    return oss.str();
}

std::string MotionDetector::formatLog(const std::string& prefix1, int value1,
                                      const std::string& prefix2, int value2,
                                      const std::string& prefix3, int value3) {
    std::ostringstream oss;
    oss << "[MotionDetector] " << prefix1 << value1 << prefix2 << value2 << prefix3 << value3;
    return oss.str();
}

std::string MotionDetector::formatLog(const std::string& prefix, const std::string& value) {
    return "[MotionDetector] " + prefix + value;
}

std::string MotionDetector::formatLog(const std::string& prefix1, size_t value,
                                      const std::string& prefix2) {
    std::ostringstream oss;
    oss << "[MotionDetector] " << prefix1 << value << prefix2;
    return oss.str();
}

std::string MotionDetector::formatLogFixed(const std::string& prefix, float value, int precision) {
    std::ostringstream oss;
    oss << "[MotionDetector] " << prefix << std::fixed << std::setprecision(precision) << value;
    return oss.str();
}

std::string MotionDetector::formatLogFixed(const std::string& prefix1, int value,
                                           const std::string& prefix2, float floatValue,
                                           int precision) {
    std::ostringstream oss;
    oss << "[MotionDetector] " << prefix1 << value << prefix2 << std::fixed
        << std::setprecision(precision) << floatValue;
    return oss.str();
}
}  // namespace KeyFrame