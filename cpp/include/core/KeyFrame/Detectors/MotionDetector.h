#pragma once

#include <map>
#include <memory>
#include <opencv2/core/mat.hpp>
#include <opencv2/core/types.hpp>
#include <opencv2/opencv.hpp>
#include <string>
#include <vector>

#include "DataConverter.h"
#include "ModelManager.h"

namespace KeyFrame {

class FrameResource;  // 前向声明

class MotionDetector {
public:
    struct Config {
        float confidenceThreshold = 0.25f;  // 检测置信度阈值
        float nmsThreshold = 0.45f;         // 非极大值抑制阈值
        int inputWidth = 640;               // 模型输入宽度
        int maxTrackedObjects = 50;         // 最大跟踪对象数

        // ByteTrack参数
        float trackHighThreshold = 0.6f;  // 跟踪高置信度阈值
        float trackLowThreshold = 0.1f;   // 跟踪低置信度阈值
        int trackBufferSize = 30;         // 跟踪缓冲区大小

        // 运动评分权重
        float pixelMotionWeight = 0.8f;   // 帧差法权重 (80%)
        float objectMotionWeight = 0.2f;  // YOLO目标检测权重 (20%)
    };

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
    // 内部辅助函数
    std::vector<float> preprocessFrame(const cv::Mat& frame);
    std::vector<Detection> postprocessDetections(const std::vector<std::vector<float>>& outputs,
                                                 const cv::Size& originalSize);
    void UpdateTracks(const std::vector<Detection>& detections, int& newTracks, int& lostTracks);
    void updateTrackInfo(Track& track, const Detection& det);
    float ComputeMotionScore(const std::vector<Track>& tracks, int newTracks, int lostTracks,
                             float pixelMotion);
    float calculateIOU(const cv::Rect& box1, const cv::Rect& box2);

    // (新增) 计算像素级运动
    float calculatePixelMotion(const cv::Mat& frame);

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
};

}  // namespace KeyFrame