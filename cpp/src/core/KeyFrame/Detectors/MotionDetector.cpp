#include "MotionDetector.h"

#include <algorithm>
#include <cstddef>
#include <exception>
#include <iomanip>
#include <ios>
#include <memory>
#include <opencv2/core.hpp>
#include <opencv2/core/mat.hpp>
#include <opencv2/core/types.hpp>
#include <opencv2/dnn/dnn.hpp>
#include <opencv2/imgproc.hpp>  // Added for image processing functions
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
    const cv::Mat& frame = resource->getOriginalFrame();
    // 输入验证
    if (frame.empty()) {
        LOG_WARN("[MotionDetector] 输入帧为空，返回默认结果");
        return Result{0.0f, {}, 0, 0, 0.0f, 0.0f};
    }

    // 预处理 (使用 FrameResource 缓存)
    std::string cacheKey = "motion_tensor_" + std::to_string(config_.inputWidth);
    auto cachedTensor = resource->getOrGenerate<std::vector<float>>(
        cacheKey, [&]() { return std::make_shared<std::vector<float>>(preprocessFrame(frame)); });

    if (!cachedTensor || cachedTensor->empty()) {
        LOG_ERROR("[MotionDetector] 预处理失败，返回默认结果");
        return Result{0.0f, activeTracks_, 0, 0, 0.0f, 0.0f};
    }

    std::vector<float> inputData = *cachedTensor;

    // 模型推理
    std::vector<std::vector<float>> outputs = modelManager_.runInference(modelName_, {inputData});
    if (outputs.empty() || outputs[0].empty()) {
        LOG_ERROR("[MotionDetector] 模型推理失败或输出为空");
        return Result{0.0f, activeTracks_, 0, 0, 0.0f, 0.0f};
    }
    {
        std::ostringstream oss;
        oss << "[MotionDetector] 推理完成，输出大小: " << outputs[0].size();
        LOG_DEBUG(oss.str());
    }

    // 后处理
    std::vector<Detection> detections = postprocessDetections(outputs, frame.size());
    {
        std::ostringstream oss;
        oss << "[MotionDetector] 检测到 " << detections.size() << " 个目标";
        LOG_DEBUG(oss.str());
    }

    // 更新跟踪器
    int newTracks = 0;
    int lostTracks = 0;
    UpdateTracks(detections, newTracks, lostTracks);
    {
        std::ostringstream oss;
        oss << "[MotionDetector] 跟踪更新完成 - 活跃: " << activeTracks_.size()
            << ", 新增: " << newTracks << ", 丢失: " << lostTracks;
        LOG_DEBUG(oss.str());
    }

    // (新增) 计算像素级运动
    float pixelMotion = calculatePixelMotion(frame);

    // 计算运动得分 (传入像素运动)
    float score = ComputeMotionScore(activeTracks_, newTracks, lostTracks, pixelMotion);

    // 计算平均速度
    float totalVelocity = 0.0f;
    for (const auto& track : activeTracks_) {
        totalVelocity += cv::norm(track.Velocity);
    }
    float avgVelocity = activeTracks_.empty() ? 0.0f : totalVelocity / activeTracks_.size();

    {
        std::ostringstream oss;
        oss << "[MotionDetector] 检测完成 - 得分: " << std::fixed << std::setprecision(3) << score
            << ", 像素运动: " << pixelMotion << ", 平均速度: " << std::setprecision(2)
            << avgVelocity;
        LOG_INFO(oss.str());
    }

    return Result{score, activeTracks_, newTracks, lostTracks, avgVelocity, pixelMotion};
}

float MotionDetector::calculatePixelMotion(const cv::Mat& frame) {
    if (frame.empty())
        return 0.0f;

    // 1. 降采样：处理 640 宽度的图像足够了，速度提升 16 倍 (针对 2.5K 屏幕)
    cv::Mat smallFrame;
    cv::resize(frame, smallFrame, cv::Size(640, 360));

    // 转灰度
    cv::Mat gray;
    if (smallFrame.channels() == 3) {
        cv::cvtColor(smallFrame, gray, cv::COLOR_BGR2GRAY);
    } else {
        gray = smallFrame.clone();
    }

    // 2. 减小模糊核：从 21x21 降到 5x5，保留更多细节
    cv::GaussianBlur(gray, gray, cv::Size(5, 5), 0);

    // 第一帧特殊处理 (增加尺寸检查防止崩溃)
    if (prevGrayFrame_.empty() || prevGrayFrame_.size() != gray.size()) {
        prevGrayFrame_ = gray;
        return 0.0f;
    }

    // 计算帧差
    cv::Mat diff;
    cv::absdiff(prevGrayFrame_, gray, diff);

    // 二值化阈值处理 (阈值25)
    cv::threshold(diff, diff, 25, 255, cv::THRESH_BINARY);

    // 3. 形态学处理：消除噪点并连接区域
    cv::Mat kernel = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(3, 3));
    cv::erode(diff, diff, kernel);   // 消除孤立小点
    cv::dilate(diff, diff, kernel);  // 增强有效运动区域

    // 计算非零像素比例
    int totalPixels = diff.cols * diff.rows;
    int motionPixels = cv::countNonZero(diff);

    // 更新上一帧
    prevGrayFrame_ = gray;

    // 4. 归一化评分 (假设 2% 的屏幕面积变化就算 1.0 满分运动)
    // 屏幕录制中 2% 的像素变化其实就很显著了
    float rawScore = static_cast<float>(motionPixels) / totalPixels;
    float score = std::min(rawScore * 50.0f, 1.0f);

    return score;
}

float MotionDetector::ComputeMotionScore(const std::vector<Track>& tracks, int newTracks,
                                         int lostTracks, float pixelMotion) {
    // 运动强度分数 = max(物体运动评分, 像素运动评分)
    // 这样主要是为了：
    // 1. 如果有识别到的物体在动，分数会高
    // 2. 如果没有识别到物体（如UI操作），但画面有变化，分数也会高

    // --- 物体运动评分计算 ---
    const float alpha = 0.3f;  // 目标数量权重
    const float beta = 0.5f;   // 平均速度权重
    const float gamma = 0.2f;  // 新增/消失目标权重

    // 1. 目标数量归一化：min(检测框数量 / 10, 1.0)
    float objectCountScore = std::min(static_cast<float>(tracks.size()) / 10.0f, 1.0f);

    // 2. 平均运动速度归一化：Σ|位移| / 目标数量
    float totalSpeed = 0.0f;
    for (const auto& track : tracks) {
        float speed = cv::norm(track.Velocity);
        totalSpeed += speed;
    }

    // 平均速度（像素/帧）
    float avgSpeed = tracks.empty() ? 0.0f : totalSpeed / tracks.size();

    // 归一化速度分数（10像素/帧对应0.5分，20像素/帧对应1.0分）
    float speedScore = std::min(avgSpeed / 20.0f, 1.0f);

    // 3. 新增/消失目标权重
    float changeScore = std::min((newTracks + lostTracks) / 10.0f, 1.0f);

    float objectMotionScore = alpha * objectCountScore + beta * speedScore + gamma * changeScore;

    // --- 最终评分融合 ---
    // 按照用户需求：帧差异(pixelMotion)占8成，YOLO(objectMotionScore)占2成
    // pixelMotion 本身代表了运动强度
    float finalScore =
        config_.pixelMotionWeight * pixelMotion + config_.objectMotionWeight * objectMotionScore;

    return std::min(finalScore, 1.0f);
}

void MotionDetector::reset() {
    {
        std::ostringstream oss;
        oss << "[MotionDetector] 重置检测器，清除 " << activeTracks_.size() << " 个活跃轨迹";
        LOG_INFO(oss.str());
    }
    activeTracks_.clear();
    lostTracks_.clear();
    trackLostFrames_.clear();
    nextTrackId_ = 0;
}

std::vector<float> MotionDetector::preprocessFrame(const cv::Mat& frame) {
    if (frame.empty()) {
        LOG_WARN("[MotionDetector] 预处理输入帧为空");
        return {};
    }

    try {
        // 使用 Letterbox 方法(保持宽高比,避免失真)
        // matToTensorLetterbox 内部已经包含了 HWC -> CHW 的转换
        auto result = DataConverter::matToTensorLetterbox(
            frame, cv::Size(config_.inputWidth, config_.inputWidth), letterboxInfo_, true,
            {0.485f, 0.456f, 0.406f}, {0.229f, 0.224f, 0.225f});

        if (result.empty()) {
            LOG_ERROR("[MotionDetector] Letterbox 转换返回空数据");
        }
        return result;
    } catch (const std::exception& e) {
        std::ostringstream oss;
        oss << "[MotionDetector] 预处理异常: " << e.what();
        LOG_ERROR(oss.str());
        return {};
    }
}

std::vector<MotionDetector::Detection> MotionDetector::postprocessDetections(
    const std::vector<std::vector<float>>& outputs, const cv::Size& originalSize) {
    // YOLOv8n 输出通常为: [1, 84, 8400]
    // 84 = 4 (bbox: cx, cy, w, h) + 80 (classes)
    const int numClasses = 80;
    const int numProposals = 8400;

    std::vector<cv::Rect> bboxes;
    std::vector<float> scores;
    std::vector<int> classIds;

    const float* output = outputs[0].data();

    // 遍历所有候选框 (注意: YOLOv8 输出通常是 [84, 8400], 需要按列遍历)
    for (int i = 0; i < numProposals; ++i) {
        // 找到最大类别分数
        float maxScore = 0.0f;
        int maxClassId = 0;

        for (int c = 0; c < numClasses; ++c) {
            float score = output[(4 + c) * numProposals + i];
            if (score > maxScore) {
                maxScore = score;
                maxClassId = c;
            }
        }

        // 置信度过滤
        if (maxScore < config_.confidenceThreshold) {
            continue;
        }

        // 解析边界框 (cx, cy, w, h) - 在 640x640 空间
        float cx = output[0 * numProposals + i];
        float cy = output[1 * numProposals + i];
        float w = output[2 * numProposals + i];
        float h = output[3 * numProposals + i];

        // 转换为 (x, y, w, h)
        int x = static_cast<int>(cx - w / 2);
        int y = static_cast<int>(cy - h / 2);
        int width = static_cast<int>(w);
        int height = static_cast<int>(h);

        // 映射回原图坐标
        cv::Rect rect = DataConverter::rescaleBox(cv::Rect(x, y, width, height), letterboxInfo_);

        bboxes.push_back(rect);
        scores.push_back(maxScore);
        classIds.push_back(maxClassId);
    }

    // NMS 去重
    std::vector<int> nmsIndices;
    cv::dnn::NMSBoxes(bboxes, scores, config_.confidenceThreshold, config_.nmsThreshold,
                      nmsIndices);

    std::vector<Detection> nmsResults;
    for (int idx : nmsIndices) {
        nmsResults.push_back({bboxes[idx], scores[idx], classIds[idx]});
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

    // 只有当轨迹已存在时才计算速度（避免新轨迹的无效速度计算）
    if (track.box.area() > 0) {
        cv::Point2f oldCenter(track.box.x + track.box.width / 2.0f,
                              track.box.y + track.box.height / 2.0f);
        cv::Point2f newVelocity = newCenter - oldCenter;

        // 速度平滑（指数移动平均）以减少噪声
        const float alpha = 0.7f;  // 新速度权重，0.7表示较快响应
        track.Velocity = alpha * newVelocity + (1.0f - alpha) * track.Velocity;

        float speed = cv::norm(track.Velocity);
        if (speed > 100.0f) {  // 异常速度检测
            std::ostringstream oss;
            oss << "[MotionDetector] 轨迹 " << track.trackId << " 速度异常: " << std::fixed
                << std::setprecision(2) << speed;
            LOG_WARN(oss.str());
        }
    } else {
        // 新轨迹初始速度为0
        track.Velocity = cv::Point2f(0, 0);
        std::ostringstream oss;
        oss << "[MotionDetector] 初始化新轨迹 " << track.trackId;
        LOG_DEBUG(oss.str());
    }

    track.box = det.box;
    track.confidence = det.confidence;
    track.classId = det.classId;
}

float MotionDetector::calculateIOU(const cv::Rect& box1, const cv::Rect& box2) {
    int interArea = (box1 & box2).area();
    int unionArea = box1.area() + box2.area() - interArea;
    if (unionArea <= 0)
        return 0.0f;
    return static_cast<float>(interArea) / unionArea;
}
}  // namespace KeyFrame