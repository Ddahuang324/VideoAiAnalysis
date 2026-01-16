#include "MotionDetector.h"

#include <algorithm>
#include <cstddef>
#include <exception>
#include <iomanip>
#include <ios>
#include <memory>
#include <mutex>
#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>
#include <sstream>
#include <string>
#include <vector>

#include "DataConverter.h"
#include "FrameResource.h"
#include "Log.h"
#include "ModelManager.h"
#include "opencv2/core/mat.hpp"
#include "opencv2/core/types.hpp"
#include "opencv2/dnn/dnn.hpp"

namespace KeyFrame {

// ========== Constructor ==========

MotionDetector::MotionDetector(ModelManager& modelManager, const Config& config,
                               const std::string& modelName)
    : modelName_(modelName.empty() ? "yolov8n.onnx" : modelName),
      modelManager_(modelManager),
      config_(config) {}

// ========== Detection ==========

MotionDetector::Result MotionDetector::detect(const cv::Mat& frame) {
    return detect(std::make_shared<FrameResource>(frame));
}

MotionDetector::Result MotionDetector::detect(std::shared_ptr<FrameResource> resource) {
    std::lock_guard<std::mutex> lock(mutex_);

    const cv::Mat& frame = resource->getOriginalFrame();
    if (frame.empty()) {
        LOG_WARN("[MotionDetector] Empty input frame");
        return Result{0.0f, {}, 0, 0, 0.0f, 0.0f};
    }

    // Preprocess frame
    std::string cacheKey = "motion_tensor_" + std::to_string(config_.inputWidth);
    auto cachedTensor = resource->getOrGenerate<std::vector<float>>(
        cacheKey, [&]() { return std::make_shared<std::vector<float>>(preprocessFrame(frame)); });

    if (!cachedTensor || cachedTensor->empty()) {
        LOG_ERROR("[MotionDetector] Preprocessing failed");
        return Result{0.0f, activeTracks_, 0, 0, 0.0f, 0.0f};
    }

    // Run inference
    std::vector<std::vector<float>> outputs =
        modelManager_.runInference(modelName_, {*cachedTensor});
    if (outputs.empty() || outputs[0].empty()) {
        LOG_ERROR("[MotionDetector] Inference failed or output empty");
        return Result{0.0f, activeTracks_, 0, 0, 0.0f, 0.0f};
    }

    LOG_DEBUG("[MotionDetector] Output size: " + std::to_string(outputs[0].size()));

    // Postprocess detections
    std::vector<Detection> detections = postprocessDetections(outputs, frame.size());
    LOG_DEBUG("[MotionDetector] Detected: " + std::to_string(detections.size()) + " objects");

    // Update tracks
    int newTracks = 0;
    int lostTracks = 0;
    UpdateTracks(detections, newTracks, lostTracks);
    LOG_DEBUG("[MotionDetector] Active: " + std::to_string(activeTracks_.size()) +
              ", New: " + std::to_string(newTracks) + ", Lost: " + std::to_string(lostTracks));

    // Compute scores
    float pixelMotion = calculatePixelMotion(frame);
    float score = ComputeMotionScore(activeTracks_, newTracks, lostTracks, pixelMotion);
    float avgVelocity = calculateAverageVelocity();

    LOG_INFO("[MotionDetector] Score: " + formatFloat(score, 3) +
             ", Pixel: " + formatFloat(pixelMotion, 3) + ", Vel: " + formatFloat(avgVelocity, 2));

    return Result{score, activeTracks_, newTracks, lostTracks, avgVelocity, pixelMotion};
}

void MotionDetector::reset() {
    std::lock_guard<std::mutex> lock(mutex_);
    LOG_INFO("[MotionDetector] Resetting, clearing " + std::to_string(activeTracks_.size()) +
             " tracks");
    activeTracks_.clear();
    lostTracks_.clear();
    trackLostFrames_.clear();
    nextTrackId_ = 0;
    prevGrayFrame_ = cv::Mat();
}

// ========== Pixel Motion Calculation ==========

float MotionDetector::calculatePixelMotion(const cv::Mat& frame) {
    if (frame.empty()) {
        return 0.0f;
    }

    // Downsample for performance
    cv::Mat smallFrame;
    cv::resize(frame, smallFrame, cv::Size(640, 360));

    // Convert to grayscale
    cv::Mat gray;
    if (smallFrame.channels() == 4) {
        cv::cvtColor(smallFrame, gray, cv::COLOR_BGRA2GRAY);
    } else if (smallFrame.channels() == 3) {
        cv::cvtColor(smallFrame, gray, cv::COLOR_BGR2GRAY);
    } else {
        gray = smallFrame.clone();
    }

    cv::GaussianBlur(gray, gray, cv::Size(5, 5), 0);

    // Initialize or check frame size
    if (prevGrayFrame_.empty() || prevGrayFrame_.size() != gray.size()) {
        prevGrayFrame_ = gray;
        return 0.0f;
    }

    // Compute difference and threshold
    cv::Mat diff;
    cv::absdiff(prevGrayFrame_, gray, diff);
    cv::threshold(diff, diff, 25, 255, cv::THRESH_BINARY);

    // Morphological operations
    cv::Mat kernel = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(3, 3));
    cv::erode(diff, diff, kernel);
    cv::dilate(diff, diff, kernel);

    int totalPixels = diff.cols * diff.rows;
    int motionPixels = cv::countNonZero(diff);

    prevGrayFrame_ = gray;

    float rawScore = static_cast<float>(motionPixels) / totalPixels;
    return std::min(rawScore * 50.0f, 1.0f);
}

// ========== Motion Score ==========

float MotionDetector::ComputeMotionScore(const std::vector<Track>& tracks, int newTracks,
                                         int lostTracks, float pixelMotion) {
    constexpr float ALPHA = 0.3f;
    constexpr float BETA = 0.5f;
    constexpr float GAMMA = 0.2f;

    float objectCountScore = std::min(static_cast<float>(tracks.size()) / 10.0f, 1.0f);
    float avgSpeed = calculateAverageVelocity();
    float speedScore = std::min(avgSpeed / 20.0f, 1.0f);
    float changeScore = std::min((newTracks + lostTracks) / 10.0f, 1.0f);

    float objectMotionScore = ALPHA * objectCountScore + BETA * speedScore + GAMMA * changeScore;
    float finalScore =
        config_.pixelMotionWeight * pixelMotion + config_.objectMotionWeight * objectMotionScore;

    return std::min(finalScore, 1.0f);
}

// ========== Preprocessing ==========

std::vector<float> MotionDetector::preprocessFrame(const cv::Mat& frame) {
    if (frame.empty()) {
        LOG_WARN("[MotionDetector] Empty frame in preprocessing");
        return {};
    }

    try {
        auto result = DataConverter::matToTensorLetterbox(
            frame, cv::Size(config_.inputWidth, config_.inputWidth), letterboxInfo_, true,
            {0.485f, 0.456f, 0.406f}, {0.229f, 0.224f, 0.225f});

        if (result.empty()) {
            LOG_ERROR("[MotionDetector] Letterbox conversion returned empty");
        }
        return result;
    } catch (const std::exception& e) {
        LOG_ERROR("[MotionDetector] Preprocessing exception: " + std::string(e.what()));
        return {};
    }
}

// ========== Postprocessing ==========

std::vector<MotionDetector::Detection> MotionDetector::postprocessDetections(
    const std::vector<std::vector<float>>& outputs, const cv::Size& originalSize) {
    if (outputs.empty() || outputs[0].empty()) {
        LOG_ERROR("[MotionDetector] Empty postprocessing input");
        return {};
    }

    constexpr int NUM_CLASSES = 80;
    const size_t totalElements = outputs[0].size();
    const int channels = NUM_CLASSES + 4;
    const int numProposals = static_cast<int>(totalElements / channels);

    if (numProposals <= 0 || totalElements % channels != 0) {
        LOG_ERROR("[MotionDetector] Invalid output size " + std::to_string(totalElements));
        return {};
    }

    std::vector<cv::Rect> bboxes;
    std::vector<float> scores;
    std::vector<int> classIds;

    const float* output = outputs[0].data();

    for (int i = 0; i < numProposals; ++i) {
        // Find max class score
        float maxScore = 0.0f;
        int maxClassId = 0;

        for (int c = 0; c < NUM_CLASSES; ++c) {
            size_t idx = static_cast<size_t>(4 + c) * numProposals + i;
            if (idx >= totalElements) {
                continue;  // Safety break
            }
            float score = output[idx];
            if (score > maxScore) {
                maxScore = score;
                maxClassId = c;
            }
        }

        if (maxScore < config_.confidenceThreshold) {
            continue;
        }

        // Extract box coordinates
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

    // Apply NMS
    std::vector<int> nmsIndices;
    cv::dnn::NMSBoxes(bboxes, scores, config_.confidenceThreshold, config_.nmsThreshold,
                      nmsIndices);

    std::vector<Detection> nmsResults;
    for (int idx : nmsIndices) {
        if (idx >= 0 && static_cast<size_t>(idx) < bboxes.size()) {
            nmsResults.push_back({bboxes[idx], scores[idx], classIds[idx]});
        }
    }

    return nmsResults;
}

// ========== Track Update ==========

void MotionDetector::UpdateTracks(const std::vector<Detection>& detections, int& newTracks,
                                  int& lostTracks) {
    std::vector<Track> nextActiveTracks;
    std::vector<bool> detectionUsed(detections.size(), false);
    newTracks = 0;
    lostTracks = 0;

    // Match existing active tracks
    for (auto& track : activeTracks_) {
        float maxIOU = 0.3f;
        int bestIdx = -1;

        for (size_t i = 0; i < detections.size(); ++i) {
            if (detectionUsed[i])
                continue;
            float iou = calculateIOU(track.box, detections[i].box);
            if (iou > maxIOU) {
                maxIOU = iou;
                bestIdx = static_cast<int>(i);
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

    // Try to recover from lost tracks
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
                bestIdx = static_cast<int>(i);
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

    // Create new tracks for unmatched detections
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

        constexpr float ALPHA = 0.7f;
        track.Velocity = ALPHA * newVelocity + (1.0f - ALPHA) * track.Velocity;

        float speed = cv::norm(track.Velocity);
        if (speed > 100.0f) {
            LOG_WARN("[MotionDetector] Track " + std::to_string(track.trackId) +
                     " abnormal speed: " + formatFloat(speed, 2));
        }
    } else {
        track.Velocity = cv::Point2f(0, 0);
    }

    track.box = det.box;
    track.confidence = det.confidence;
    track.classId = det.classId;
}

// ========== Utility Functions ==========

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

std::string MotionDetector::formatFloat(float value, int precision) {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(precision) << value;
    return oss.str();
}

}  // namespace KeyFrame
