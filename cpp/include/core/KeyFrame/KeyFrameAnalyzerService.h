#pragma once

#include <atomic>
#include <memory>
#include <string>
#include <thread>
#include <vector>

#include "IFrameAnalyzer.h"
#include "core/KeyFrame/Detectors/MotionDetector.h"
#include "core/KeyFrame/Detectors/SceneChangeDetector.h"
#include "core/KeyFrame/Detectors/TextDetector.h"
#include "core/KeyFrame/FrameAnaylzer/DynamicCalculator.h"
#include "core/KeyFrame/FrameAnaylzer/FrameScorer.h"
#include "core/KeyFrame/FrameAnaylzer/KeyFrameDetector.h"
#include "core/KeyFrame/FrameAnaylzer/StandardFrameAnalyzer.h"
#include "core/MQInfra/FrameSubscriber.h"
#include "core/MQInfra/KeyFrameMetaDataPublisher.h"
#include "core/ScreenRecorder/ProcessLayer/ThreadSafetyQueue.h"

namespace KeyFrame {

class KeyFrameAnalyzerService {
public:
    struct Config {
        // ZMQ 通信配置
        struct ZMQConfig {
            std::string frameSubEndpoint = "tcp://localhost:5555";
            std::string keyframePubEndpoint = "tcp://*:5556";
            int receiveTimeoutMs = 100;
        } zmq;

        // 模型路径配置
        struct ModelPaths {
            std::string sceneModelPath;
            std::string motionModelPath;
            std::string textDetModelPath;
            std::string textRecModelPath;
        } models;

        // 管道与线程配置
        struct PipelineConfig {
            int analysisThreadCount =
                1;  // 由于 MotionDetector 依赖顺序，默认单线程或由分析器内部并行
            int frameBufferSize = 100;
            int scoreBufferSize = 200;
        } pipeline;

        // 功能开关
        bool enableTextRecognition = false;  // 文字识别是性能杀手，默认关闭

        // 各组件具体配置
        SceneChangeDetector::Config sceneConfig;
        MotionDetector::Config motionConfig;
        TextDetector::Config textConfig;
        DynamicCalculator::Config dynamicConfig;
        FrameScorer::Config scorerConfig;
        KeyFrameDetector::Config detectorConfig;
    };

    explicit KeyFrameAnalyzerService(const Config& config);
    ~KeyFrameAnalyzerService();

    // 启动服务 (非阻塞)
    bool start();

    // 运行服务 (阻塞，通常在 main 中调用)
    void run();

    // 优雅停止
    void stop();

    bool isRunning() const { return running_; }

private:
    void initializeComponents();
    void startThreads();
    void waitThreads();

    // 管道数据项
    struct FrameItem {
        std::shared_ptr<FrameResource> resource;
        AnalysisContext context;
    };

    // 管道循环
    void receiveLoop();
    void analysisLoop();
    void selectLoop();  // 关键帧选择循环
    void publishLoop();

    Config config_;
    std::atomic<bool> running_{false};

    // 组件
    std::unique_ptr<MQInfra::FrameSubscriber> subscriber_;
    std::unique_ptr<MQInfra::KeyFrameMetaDataPublisher> publisher_;
    std::shared_ptr<StandardFrameAnalyzer> analyzer_;
    std::shared_ptr<FrameScorer> scorer_;
    std::shared_ptr<DynamicCalculator> dynamicCalculator_;
    std::shared_ptr<KeyFrameDetector> keyframeDetector_;  // 关键帧检测器

    // 内部队列
    std::unique_ptr<ThreadSafetyQueue<FrameItem>> frameQueue_;
    std::unique_ptr<ThreadSafetyQueue<FrameScore>> scoreQueue_;          // 评分队列 (所有帧)
    std::unique_ptr<ThreadSafetyQueue<FrameScore>> selectedFrameQueue_;  // 选中帧队列 (关键帧)

    // 线程句柄
    std::thread receiveThread_;
    std::vector<std::thread> analysisThreads_;
    std::thread selectThread_;  // 关键帧选择线程
    std::thread publishThread_;

    // 上下文记录
    AnalysisContext context_;
};

}  // namespace KeyFrame
