#pragma once

#include <map>
#include <memory>
#include <mutex>
#include <opencv2/core/mat.hpp>
#include <opencv2/core/types.hpp>
#include <opencv2/opencv.hpp>
#include <string>
#include <vector>

#include "DataConverter.h"
#include "ModelManager.h"
#include "core/Config/UnifiedConfig.h"


namespace KeyFrame {

class FrameResource;  // 前向声明

// 使用统一配置系统的类型别名
using MotionDetectorConfig = Config::MotionDetectorConfig;

class MotionDetector {
public:
    // 使用统一配置
    using Config = MotionDetectorConfig;

    struct Detection {
        cv::Rect box;      // 目标边界框
        float confidence;  // 目标置信度
        int classId;       // 目标类别ID
    };

    struct Track {
        int trackId;           // 跟踪ID
        cv::Rect box;          // 目标边界框
        cv::Point2f Velocity;  // 目标速度
        float confidence;      // 目标置信度
        int classId;           // 目标类别ID
    };

    struct Result {
        float score;                    // 综合运动评分 [0.0, 1.0]
        std::vector<Track> track;       // 当前活跃的跟踪轨迹
        int newTracks;                  // 新增轨迹数量
        int LostTracks;                 // 丢失轨迹数量
        float avgVelocity;              // 平均轨迹速度
        float pixelMotionScore = 0.0f;  // (新增) 像素级运动评分，用于捕获非物体运动(如UI变化)
    };

    /**
     * @brief 构造函数
     * @param modelManager 模型管理器引用
     * @param config 配置参数
     * @param modelName 模型名称 (默认 yolov8n.onnx)
     */
    MotionDetector(ModelManager& modelManager, const Config& config,
                   const std::string& modelName = "yolov8n.onnx");

    ~MotionDetector() = default;

    /**
     * @brief 执行运动检测
     * @param frame 输入视频帧
     * @return 检测结果
     */
    Result detect(const cv::Mat& frame);
    Result detect(std::shared_ptr<FrameResource> resource);

    /**
     * @brief 重置检测器状态
     */
    void reset();

    /**
     * @brief 获取当前所有活跃轨迹
     */
    const std::vector<Track>& GetTracks() const { return activeTracks_; }

private:
    std::vector<float> preprocessFrame(const cv::Mat& frame);
    std::vector<Detection> postprocessDetections(const std::vector<std::vector<float>>& outputs,
                                                 const cv::Size& originalSize);
    void UpdateTracks(const std::vector<Detection>& detections, int& newTracks, int& lostTracks);
    void updateTrackInfo(Track& track, const Detection& det);
    float ComputeMotionScore(const std::vector<Track>& tracks, int newTracks, int lostTracks,
                             float pixelMotion);
    float calculateIOU(const cv::Rect& box1, const cv::Rect& box2);
    float calculatePixelMotion(const cv::Mat& frame);
    float calculateAverageVelocity();
    std::string formatFloat(float value, int precision);

    std::string modelName_;
    ModelManager& modelManager_;
    Config config_;

    // 状态变量
    std::vector<Track> activeTracks_;
    std::vector<Track> lostTracks_;
    std::map<int, int> trackLostFrames_;  // 记录每个 Track 丢失了多少帧
    int nextTrackId_ = 0;

    // (新增) 像素运动检测状态
    cv::Mat prevGrayFrame_;

    // Letterbox 变换信息 (用于坐标还原)
    DataConverter::LetterboxInfo letterboxInfo_;
    mutable std::mutex mutex_;
};

}  // namespace KeyFrame
