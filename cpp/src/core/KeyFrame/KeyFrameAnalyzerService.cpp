#include "core/KeyFrame/KeyFrameAnalyzerService.h"

#include <opencv2/core/hal/interface.h>

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <mutex>
#include <opencv2/core/mat.hpp>
#include <opencv2/opencv.hpp>
#include <string>
#include <thread>
#include <vector>

#include "DynamicCalculator.h"
#include "FrameScorer.h"
#include "FrameSubscriber.h"
#include "IFrameAnalyzer.h"
#include "KeyFrameDetector.h"
#include "KeyFrameMetaDataPublisher.h"
#include "Log.h"
#include "MotionDetector.h"
#include "SceneChangeDetector.h"
#include "StandardFrameAnalyzer.h"
#include "TextDetector.h"
#include "ThreadSafetyQueue.h"
#include "core/KeyFrame/Foundation/ModelManager.h"
#include "core/KeyFrame/FrameAnalyzer/FrameResource.h"
#include "core/MQInfra/Protocol.h"

namespace KeyFrame {

KeyFrameAnalyzerService::KeyFrameAnalyzerService(const Config& config) : config_(config) {
    initializeComponents();
}

KeyFrameAnalyzerService::~KeyFrameAnalyzerService() {
    stop();
}

void KeyFrameAnalyzerService::initializeComponents() {
    LOG_INFO("Initializing KeyFrameAnalyzerService components...");

    // 1. 初始化 ZMQ
    subscriber_ = std::make_unique<MQInfra::FrameSubscriber>();
    if (!subscriber_->initialize(config_.zmq.frameSubEndpoint)) {
        LOG_ERROR("Failed to initialize FrameSubscriber at " + config_.zmq.frameSubEndpoint);
    }

    publisher_ = std::make_unique<MQInfra::KeyFrameMetaDataPublisher>();
    if (!publisher_->initialize(config_.zmq.keyframePubEndpoint)) {
        LOG_ERROR("Failed to initialize KeyFrameMetaDataPublisher at " +
                  config_.zmq.keyframePubEndpoint);
    }

    // 2. 加载模型到 ModelManager
    auto& modelMgr = ModelManager::GetInstance();
    if (!config_.models.sceneModelPath.empty()) {
        modelMgr.loadModel("MobileNet-v3-Small", config_.models.sceneModelPath,
                           ModelManager::FrameWorkType::ONNXRuntime);
    }
    if (!config_.models.motionModelPath.empty()) {
        modelMgr.loadModel("yolov8n.onnx", config_.models.motionModelPath,
                           ModelManager::FrameWorkType::ONNXRuntime);
    }
    if (!config_.models.textDetModelPath.empty()) {
        modelMgr.loadModel("ch_PP-OCRv4_det_infer.onnx", config_.models.textDetModelPath,
                           ModelManager::FrameWorkType::ONNXRuntime);
    }
    // ⚠️ 文字识别模型是性能杀手，仅在显式启用时才加载
    if (config_.enableTextRecognition && !config_.models.textRecModelPath.empty()) {
        modelMgr.loadModel("ch_PP-OCRv4_rec_infer.onnx", config_.models.textRecModelPath,
                           ModelManager::FrameWorkType::ONNXRuntime);
        LOG_INFO("Text recognition model loaded. (Performance warning!)");
    } else {
        LOG_INFO("Text recognition disabled (enableTextRecognition=false)");
    }

    // 3. 初始化分析器组件
    auto sceneDetector = std::make_shared<SceneChangeDetector>(modelMgr, config_.sceneConfig);
    auto motionDetector = std::make_shared<MotionDetector>(modelMgr, config_.motionConfig);
    // 传入 TextDetector 配置，确保其也知道是否启用识别
    auto textConfig = config_.textConfig;
    textConfig.enableRecognition = config_.enableTextRecognition;
    auto textDetector = std::make_shared<TextDetector>(modelMgr, textConfig);

    analyzer_ =
        std::make_shared<StandardFrameAnalyzer>(sceneDetector, motionDetector, textDetector);
    dynamicCalculator_ = std::make_shared<DynamicCalculator>(config_.dynamicConfig);
    scorer_ = std::make_shared<FrameScorer>(dynamicCalculator_, config_.scorerConfig);
    keyframeDetector_ = std::make_shared<KeyFrameDetector>(config_.detectorConfig);

    // 4. 初始化队列
    frameQueue_ = std::make_unique<ThreadSafetyQueue<FrameItem>>(config_.pipeline.frameBufferSize);
    scoreQueue_ = std::make_unique<ThreadSafetyQueue<FrameScore>>(config_.pipeline.scoreBufferSize);
    selectedFrameQueue_ =
        std::make_unique<ThreadSafetyQueue<FrameScore>>(config_.pipeline.scoreBufferSize);

    LOG_INFO("KeyFrameAnalyzerService components initialized.");
}

bool KeyFrameAnalyzerService::start() {
    if (running_)
        return true;
    running_ = true;
    startThreads();
    return true;
}

void KeyFrameAnalyzerService::run() {
    if (start()) {
        waitThreads();
    }
}

void KeyFrameAnalyzerService::stop() {
    if (!running_)
        return;
    running_ = false;

    // 1. 先停止接收线程
    if (subscriber_) {
        subscriber_->shutdown();
    }
    if (receiveThread_.joinable()) {
        receiveThread_.join();
    }

    // 2. 停止 frameQueue，唤醒并等待分析线程
    if (frameQueue_) {
        frameQueue_->stop();
    }
    for (auto& t : analysisThreads_) {
        if (t.joinable())
            t.join();
    }

    // 3. 停止 scoreQueue，唤醒并等待选择线程 (选择线程退出前会 Flush 缓冲区)
    if (scoreQueue_) {
        scoreQueue_->stop();
    }
    if (selectThread_.joinable()) {
        selectThread_.join();
    }

    // 4. 停止 selectedFrameQueue，唤醒并等待发布线程
    if (selectedFrameQueue_) {
        selectedFrameQueue_->stop();
    }
    if (publishThread_.joinable()) {
        publishThread_.join();
    }

    // 5. 最后关闭发布者
    if (publisher_) {
        publisher_->shutdown();
    }

    LOG_INFO("KeyFrameAnalyzerService stopped.");
}

void KeyFrameAnalyzerService::startThreads() {
    receiveThread_ = std::thread(&KeyFrameAnalyzerService::receiveLoop, this);

    int threadCount = config_.pipeline.analysisThreadCount;
    for (int i = 0; i < threadCount; ++i) {
        analysisThreads_.emplace_back(&KeyFrameAnalyzerService::analysisLoop, this);
    }

    selectThread_ = std::thread(&KeyFrameAnalyzerService::selectLoop, this);
    publishThread_ = std::thread(&KeyFrameAnalyzerService::publishLoop, this);
}

void KeyFrameAnalyzerService::waitThreads() {
    if (receiveThread_.joinable())
        receiveThread_.join();
    for (auto& t : analysisThreads_) {
        if (t.joinable())
            t.join();
    }
    if (selectThread_.joinable())
        selectThread_.join();
    if (publishThread_.joinable())
        publishThread_.join();
}

AnalysisContext KeyFrameAnalyzerService::getContext() const {
    std::lock_guard<std::mutex> lock(contextMutex_);
    return context_;
}

std::vector<FrameScore> KeyFrameAnalyzerService::getLatestKeyFrames() const {
    std::lock_guard<std::mutex> lock(statsMutex_);
    return latestKeyFrames_;
}

void KeyFrameAnalyzerService::updateLatestKeyFrames(const FrameScore& score) {
    std::lock_guard<std::mutex> lock(statsMutex_);
    latestKeyFrames_.push_back(score);
    totalKeyFrames_++;
    if (latestKeyFrames_.size() > 20) {
        latestKeyFrames_.erase(latestKeyFrames_.begin());
    }
}

void KeyFrameAnalyzerService::receiveLoop() {
    LOG_INFO("Receive loop started.");
    while (running_) {
        auto msg = subscriber_->receiveFrame(config_.zmq.receiveTimeoutMs);
        if (!msg)
            continue;

        // 转换为 cv::Mat
        int type = (msg->header.channels == 3) ? CV_8UC3 : CV_8UC1;
        cv::Mat mat(msg->header.height, msg->header.width, type, (void*)msg->image_data.data());

        // 创建 FrameResource (需要 clone 因为 msg 可能释放)
        auto resource = std::make_shared<FrameResource>(mat.clone());

        // 构建上下文
        AnalysisContext ctx;
        ctx.frameIndex = msg->header.frameID;
        ctx.timestamp = static_cast<double>(msg->header.timestamp) / 1000.0;
        ctx.frameSize = mat.size();

        // 更新服务全局上下文
        {
            std::lock_guard<std::mutex> lock(contextMutex_);
            context_.frameIndex = ctx.frameIndex;
            context_.timestamp = ctx.timestamp;
            context_.frameSize = ctx.frameSize;
            context_.totalFramesAnalyzed++;
        }

        if (!frameQueue_->push({resource, ctx}, std::chrono::milliseconds(100))) {
            LOG_WARN("Frame queue full, dropping frame " + std::to_string(msg->header.frameID));
        }
    }
}

void KeyFrameAnalyzerService::analysisLoop() {
    LOG_INFO("Analysis loop started.");
    while (running_ || !frameQueue_->empty()) {
        FrameItem item;
        if (frameQueue_->pop(item, std::chrono::milliseconds(100))) {
            auto scores = analyzer_->analyzeFrame(item.resource, item.context);
            auto finalScore = scorer_->score(scores, item.context);

            if (!scoreQueue_->push(finalScore, std::chrono::milliseconds(100))) {
                LOG_WARN("Score queue full, dropping score for frame " +
                         std::to_string(finalScore.frameIndex));
            }
        }
    }
}

void KeyFrameAnalyzerService::selectLoop() {
    LOG_INFO("Select loop started (adaptive selection strategy).");

    std::vector<FrameScore> scoreBuffer;
    const size_t WINDOW_SIZE = 30;

    while (running_ || !scoreQueue_->empty()) {
        FrameScore score;
        if (scoreQueue_->pop(score, std::chrono::milliseconds(100))) {
            scoreBuffer.push_back(score);

            if (scoreBuffer.size() >= WINDOW_SIZE) {
                const auto& detConfig = config_.detectorConfig;
                int dynamicK = -1;
                if (!detConfig.useThresholdMode) {
                    dynamicK =
                        static_cast<int>(scoreBuffer.size() * detConfig.targetCompressionRatio);
                    dynamicK = std::max(detConfig.minKeyFrameCount,
                                        std::min(detConfig.maxKeyFrameCount, dynamicK));
                }

                auto selectionResult = keyframeDetector_->selectFromFrames(scoreBuffer, dynamicK);

                for (const auto& selectedScore : selectionResult.KeyframeScores) {
                    updateLatestKeyFrames(selectedScore);
                    if (!selectedFrameQueue_->push(selectedScore, std::chrono::milliseconds(100))) {
                        LOG_WARN("Selected frame queue full, dropping keyframe " +
                                 std::to_string(selectedScore.frameIndex));
                    }
                }

                LOG_DEBUG("Keyframe selection: " + std::to_string(selectionResult.selectedFrames) +
                          " / " + std::to_string(selectionResult.totalFrames) +
                          " frames selected. " +
                          (detConfig.useThresholdMode ? "[Threshold Mode]" : "[Top-K Mode]"));

                scoreBuffer.clear();
            }
        }
    }

    // 处理剩余的缓冲帧（进程退出时）
    if (!scoreBuffer.empty()) {
        const auto& detConfig = config_.detectorConfig;
        int dynamicK = -1;
        if (!detConfig.useThresholdMode) {
            dynamicK = static_cast<int>(scoreBuffer.size() * detConfig.targetCompressionRatio);
            dynamicK = std::max(detConfig.minKeyFrameCount,
                                std::min(detConfig.maxKeyFrameCount, dynamicK));
        }

        auto selectionResult = keyframeDetector_->selectFromFrames(scoreBuffer, dynamicK);
        for (const auto& selectedScore : selectionResult.KeyframeScores) {
            selectedFrameQueue_->push(selectedScore, std::chrono::milliseconds(100));
        }
    }

    LOG_INFO("Select loop stopped.");
}

void KeyFrameAnalyzerService::publishLoop() {
    LOG_INFO("Publish loop started.");
    while (running_ || !selectedFrameQueue_->empty()) {
        FrameScore score;
        if (selectedFrameQueue_->pop(score, std::chrono::milliseconds(100))) {
            Protocol::KeyFrameMetaDataMessage meta;
            meta.header.frameID = score.frameIndex;
            meta.header.timestamp = static_cast<uint64_t>(score.timestamp * 1000.0);
            meta.header.Final_Score = score.finalscore;
            meta.header.Sence_Score = score.sceneContribution;
            meta.header.Motion_Score = score.motionContribution;
            meta.header.Text_Score = score.textContribution;
            meta.header.is_Scene_Change = score.rawScores.sceneChangeResult.isSceneChange ? 1 : 0;

            // 计算 CRC
            meta.crc32 = Protocol::calculateCrc32(&meta.header, sizeof(meta.header));

            publisher_->publish(meta);
        }
    }
}

}  // namespace KeyFrame