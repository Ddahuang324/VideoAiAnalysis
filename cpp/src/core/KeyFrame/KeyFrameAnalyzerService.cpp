#include "core/KeyFrame/KeyFrameAnalyzerService.h"

#include <algorithm>
#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <mutex>
#include <opencv2/core/mat.hpp>
#include <opencv2/opencv.hpp>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include "DynamicCalculator.h"
#include "FrameScorer.h"
#include "FrameSubscriber.h"
#include "IFrameAnalyzer.h"
#include "KeyFrameDetector.h"
#include "KeyFrameMetaDataPublisher.h"
#include "KeyFrameVideoEncoder.h"
#include "Log.h"
#include "MotionDetector.h"
#include "SceneChangeDetector.h"
#include "StandardFrameAnalyzer.h"
#include "TextDetector.h"
#include "ThreadSafetyQueue.h"
#include "core/KeyFrame/Foundation/ModelManager.h"
#include "core/KeyFrame/FrameAnalyzer/FrameResource.h"
#include "core/MQInfra/Protocol.h"
#include "opencv2/core/hal/interface.h"
#include "opencv2/videoio.hpp"

namespace KeyFrame {

// ========== Constructor / Destructor ==========

KeyFrameAnalyzerService::KeyFrameAnalyzerService(const Config& config) : config_(config) {
    initializeComponents();
}

KeyFrameAnalyzerService::~KeyFrameAnalyzerService() {
    stop();
}

// ========== Initialization ==========

void KeyFrameAnalyzerService::initializeComponents() {
    LOG_INFO("[KeyFrameAnalyzerService] Initializing components...");

    // 1. Initialize ZMQ
    subscriber_ = std::make_unique<MQInfra::FrameSubscriber>();
    if (!subscriber_->initialize(config_.zmqSubscriber.endpoint)) {
        LOG_ERROR("[KeyFrameAnalyzerService] Failed to initialize FrameSubscriber at " +
                  config_.zmqSubscriber.endpoint);
    }

    publisher_ = std::make_unique<MQInfra::KeyFrameMetaDataPublisher>();
    if (!publisher_->initialize(config_.zmqPublisher.endpoint)) {
        LOG_ERROR("[KeyFrameAnalyzerService] Failed to initialize KeyFrameMetaDataPublisher at " +
                  config_.zmqPublisher.endpoint);
    }

    // 2. Load models into ModelManager
    auto& modelMgr = ModelManager::GetInstance();

    if (!config_.models.sceneModelPath.empty()) {
        auto fullPath =
            std::filesystem::path(config_.models.basePath) / config_.models.sceneModelPath;
        modelMgr.loadModel("MobileNet-v3-Small", fullPath.string(),
                           ModelManager::FrameWorkType::ONNXRuntime);
    }

    if (!config_.models.motionModelPath.empty()) {
        auto fullPath =
            std::filesystem::path(config_.models.basePath) / config_.models.motionModelPath;
        modelMgr.loadModel("yolov8n.onnx", fullPath.string(),
                           ModelManager::FrameWorkType::ONNXRuntime);
    }

    if (!config_.models.textDetModelPath.empty()) {
        auto fullPath =
            std::filesystem::path(config_.models.basePath) / config_.models.textDetModelPath;
        modelMgr.loadModel("ch_PP-OCRv4_det_infer.onnx", fullPath.string(),
                           ModelManager::FrameWorkType::ONNXRuntime);
    }

    if (config_.enableTextRecognition && !config_.models.textRecModelPath.empty()) {
        auto fullPath =
            std::filesystem::path(config_.models.basePath) / config_.models.textRecModelPath;
        modelMgr.loadModel("ch_PP-OCRv4_rec_infer.onnx", fullPath.string(),
                           ModelManager::FrameWorkType::ONNXRuntime);
        LOG_INFO("[KeyFrameAnalyzerService] Text recognition model loaded (performance warning)");
    } else {
        LOG_INFO("[KeyFrameAnalyzerService] Text recognition disabled");
    }

    // 3. Initialize detectors with unified config
    auto sceneDetector = std::make_shared<SceneChangeDetector>(
        modelMgr, SceneChangeDetector::Config{
                      config_.sceneDetector.similarityThreshold, config_.sceneDetector.featureDim,
                      config_.sceneDetector.inputSize, config_.sceneDetector.enableCache});

    auto motionDetector = std::make_shared<MotionDetector>(
        modelMgr,
        MotionDetector::Config{
            config_.motionDetector.confidenceThreshold, config_.motionDetector.nmsThreshold,
            config_.motionDetector.inputWidth, config_.motionDetector.maxTrackedObjects,
            config_.motionDetector.trackHighThreshold, config_.motionDetector.trackLowThreshold,
            config_.motionDetector.trackBufferSize, config_.motionDetector.pixelMotionWeight,
            config_.motionDetector.objectMotionWeight});

    auto textDetector = std::make_shared<TextDetector>(
        modelMgr,
        TextDetector::Config{
            config_.textDetector.detInputHeight, config_.textDetector.detInputWidth,
            config_.textDetector.recInputHeight, config_.textDetector.recInputWidth,
            config_.textDetector.detThreshold, config_.textDetector.recThreshold,
            config_.enableTextRecognition, config_.textDetector.alpha, config_.textDetector.beta});

    analyzer_ =
        std::make_shared<StandardFrameAnalyzer>(sceneDetector, motionDetector, textDetector);

    // 4. Initialize scoring and selection components
    dynamicCalculator_ = std::make_shared<DynamicCalculator>(DynamicCalculator::Config{
        config_.dynamicCalculator.baseWeights, config_.dynamicCalculator.currentFrameWeight,
        config_.dynamicCalculator.activationInfluence, config_.dynamicCalculator.historyWindowSize,
        config_.dynamicCalculator.minWeight, config_.dynamicCalculator.maxWeight});

    scorer_ = std::make_shared<FrameScorer>(
        dynamicCalculator_,
        FrameScorer::Config{
            config_.frameScorer.enableDynamicWeighting, config_.frameScorer.enableSmoothing,
            config_.frameScorer.smoothingWindowSize, config_.frameScorer.smoothingEMAAlpha,
            config_.frameScorer.sceneChangeBoost, config_.frameScorer.motionIncreaseBoost,
            config_.frameScorer.textIncreaseBoost});

    keyframeDetector_ = std::make_shared<KeyFrameDetector>(KeyFrameDetector::Config{
        config_.keyframeDetector.targetKeyFrameCount,
        config_.keyframeDetector.targetCompressionRatio, config_.keyframeDetector.minKeyFrameCount,
        config_.keyframeDetector.maxKeyFrameCount, config_.keyframeDetector.minTemporalDistance,
        config_.keyframeDetector.useThresholdMode, config_.keyframeDetector.highQualityThreshold,
        config_.keyframeDetector.minScoreThreshold,
        config_.keyframeDetector.alwaysIncludeSceneChanges});

    // 5. Initialize queues
    frameQueue_ = std::make_unique<ThreadSafetyQueue<FrameItem>>(config_.pipeline.frameBufferSize);
    scoreQueue_ = std::make_unique<ThreadSafetyQueue<FrameScore>>(config_.pipeline.scoreBufferSize);
    selectedFrameQueue_ =
        std::make_unique<ThreadSafetyQueue<FrameScore>>(config_.pipeline.scoreBufferSize);

    // Initialize keyframe video encoder
    keyframeEncoder_ = std::make_unique<KeyFrameVideoEncoder>();

    LOG_INFO("[KeyFrameAnalyzerService] Components initialized");
}

// ========== Service Control ==========

bool KeyFrameAnalyzerService::start() {
    if (running_) {
        return true;
    }
    running_ = true;
    startThreads();
    return true;
}

void KeyFrameAnalyzerService::run() {
    if (start()) {
        waitThreads();
    }
}

bool KeyFrameAnalyzerService::analyzeVideoFile(const std::string& filePath) {
    if (running_) {
        LOG_WARN("[KeyFrameAnalyzerService] Service is already running");
        return false;
    }

    if (!std::filesystem::exists(filePath)) {
        LOG_ERROR("[KeyFrameAnalyzerService] Video file not found: " + filePath);
        return false;
    }

    isOfflineMode_ = true;
    running_ = true;
    currentSourceVideoPath_ = filePath;
    selectedKeyFrameIndices_.clear();
    eosReceived_ = false;  // Reset EOS flag for new analysis

    // Start processing threads first
    int threadCount = config_.pipeline.analysisThreadCount;
    for (int i = 0; i < threadCount; ++i) {
        analysisThreads_.emplace_back(&KeyFrameAnalyzerService::analysisLoop, this);
    }
    selectThread_ = std::thread(&KeyFrameAnalyzerService::selectLoop, this);
    publishThread_ = std::thread(&KeyFrameAnalyzerService::publishLoop, this);

    // Start reader thread
    fileReadThread_ = std::thread(&KeyFrameAnalyzerService::fileReadLoop, this, filePath);

    LOG_INFO("[KeyFrameAnalyzerService] Started offline analysis for: " + filePath);
    return true;
}

void KeyFrameAnalyzerService::setKeyFrameVideoCallback(KeyFrameVideoCallback callback) {
    std::lock_guard<std::mutex> lock(callbackMutex_);
    keyframeVideoCallback_ = std::move(callback);
}

void KeyFrameAnalyzerService::stop() {
    if (!running_) {
        return;
    }
    running_ = false;

    // Stop in reverse order: receive -> analysis -> select -> publish

    if (subscriber_) {
        subscriber_->shutdown();
    }
    if (receiveThread_.joinable()) {
        receiveThread_.join();
    }

    if (frameQueue_) {
        frameQueue_->stop();
    }
    for (auto& t : analysisThreads_) {
        if (t.joinable()) {
            t.join();
        }
    }

    if (scoreQueue_) {
        scoreQueue_->stop();
    }
    if (selectThread_.joinable()) {
        selectThread_.join();
    }

    if (selectedFrameQueue_) {
        selectedFrameQueue_->stop();
    }
    if (publishThread_.joinable()) {
        publishThread_.join();
    }

    if (fileReadThread_.joinable()) {
        fileReadThread_.join();
    }

    if (publisher_) {
        publisher_->shutdown();
    }

    LOG_INFO("[KeyFrameAnalyzerService] Stopped");
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
    if (receiveThread_.joinable()) {
        receiveThread_.join();
    }
    for (auto& t : analysisThreads_) {
        if (t.joinable()) {
            t.join();
        }
    }
    if (selectThread_.joinable()) {
        selectThread_.join();
    }
    if (publishThread_.joinable()) {
        publishThread_.join();
    }
    if (fileReadThread_.joinable()) {
        fileReadThread_.join();
    }
}

// ========== State Access ==========

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

// ========== Thread Loops ==========

void KeyFrameAnalyzerService::receiveLoop() {
    LOG_INFO("[KeyFrameAnalyzerService] Receive loop started");

    while (running_ && !subscriber_->isShutdown()) {
        // Use the new generic receive
        auto result = subscriber_->receive(config_.zmqSubscriber.timeoutMs);

        // Check shutdown after receive (may have been set during blocking call)
        if (!running_ || subscriber_->isShutdown()) {
            break;
        }

        if (result.type == Protocol::ReceiveResult::Type::StopSignal) {
            LOG_INFO("[KeyFrameAnalyzerService] Received StopSignal, lastFrameId: " +
                     std::to_string(result.stopSignal->lastFrameId));

            // Push EOS marker
            AnalysisContext ctx;
            ctx.frameIndex = result.stopSignal->lastFrameId;  // Use last ID
            if (!frameQueue_->push({nullptr, ctx}, std::chrono::milliseconds(1000))) {
                LOG_ERROR("[KeyFrameAnalyzerService] Failed to push EOS to frameQueue");
            }
            continue;
        }

        if (result.type != Protocol::ReceiveResult::Type::Frame || !result.frame) {
            continue;
        }

        auto& msg = *result.frame;

        // Safety check for image data size
        int type;
        switch (msg.header.channels) {
            case 4: type = CV_8UC4; break;
            case 3: type = CV_8UC3; break;
            default: type = CV_8UC1; break;
        }
        size_t expectedSize =
            static_cast<size_t>(msg.header.width) * msg.header.height * msg.header.channels;

        if (msg.image_data.size() < expectedSize) {
            LOG_ERROR("[KeyFrameAnalyzerService] Received truncated frame " +
                      std::to_string(msg.header.frameID));
            continue;
        }

        // Convert to cv::Mat
        cv::Mat mat(msg.header.height, msg.header.width, type, (void*)msg.image_data.data());

        auto resource = std::make_shared<FrameResource>(mat.clone());

        AnalysisContext ctx;
        ctx.frameIndex = msg.header.frameID;
        ctx.timestamp = static_cast<double>(msg.header.timestamp) / 1000.0;
        ctx.frameSize = mat.size();

        {
            std::lock_guard<std::mutex> lock(contextMutex_);
            context_.frameIndex = ctx.frameIndex;
            context_.timestamp = ctx.timestamp;
            context_.frameSize = ctx.frameSize;
            context_.totalFramesAnalyzed++;
        }

        if (!frameQueue_->push({resource, ctx}, std::chrono::milliseconds(100))) {
            if (isOfflineMode_) {
                // In offline mode, retry until success instead of dropping
                while (running_ &&
                       !frameQueue_->push({resource, ctx}, std::chrono::milliseconds(100))) {
                    std::this_thread::yield();
                }
            } else {
                LOG_WARN("[KeyFrameAnalyzerService] Frame queue full, dropping " +
                         std::to_string(msg.header.frameID));
            }
        }
    }
}

void KeyFrameAnalyzerService::fileReadLoop(const std::string& filePath) {
    LOG_INFO("[KeyFrameAnalyzerService] File read loop started: " + filePath);

    cv::VideoCapture cap(filePath);
    if (!cap.isOpened()) {
        LOG_ERROR("[KeyFrameAnalyzerService] Failed to open video file: " + filePath);
        running_ = false;
        return;
    }

    double fps = cap.get(cv::CAP_PROP_FPS);
    if (fps <= 0)
        fps = 30.0;
    int totalFrames = static_cast<int>(cap.get(cv::CAP_PROP_FRAME_COUNT));
    int frameIdx = 0;

    while (running_) {
        cv::Mat frame;
        if (!cap.read(frame)) {
            LOG_INFO("[KeyFrameAnalyzerService] Reached end of video file");
            break;
        }

        auto resource = std::make_shared<FrameResource>(frame.clone());

        AnalysisContext ctx;
        ctx.frameIndex = frameIdx;
        ctx.timestamp = static_cast<double>(frameIdx) / fps;
        ctx.frameSize = frame.size();

        {
            std::lock_guard<std::mutex> lock(contextMutex_);
            context_.frameIndex = ctx.frameIndex;
            context_.timestamp = ctx.timestamp;
            context_.frameSize = ctx.frameSize;
            context_.totalFramesAnalyzed++;
        }

        // Push to queue, wait if full in offline mode
        while (running_ && !frameQueue_->push({resource, ctx}, std::chrono::milliseconds(100))) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }

        frameIdx++;
        if (frameIdx % 100 == 0) {
            LOG_INFO("[KeyFrameAnalyzerService] Progressive: " + std::to_string(frameIdx) + " / " +
                     std::to_string(totalFrames) + " frames read");
        }
    }

    // Push EOS marker
    AnalysisContext eosCtx;
    eosCtx.frameIndex = frameIdx - 1;
    while (running_ && !frameQueue_->push({nullptr, eosCtx}, std::chrono::milliseconds(1000))) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    LOG_INFO("[KeyFrameAnalyzerService] File read loop finished. Total frames: " +
             std::to_string(frameIdx));

    // Wait for EOS to propagate through the entire pipeline
    {
        std::unique_lock<std::mutex> lock(eosMutex_);
        eosCondition_.wait(lock, [this] { return eosReceived_ || !running_; });
    }
    LOG_INFO("[KeyFrameAnalyzerService] EOS received, proceeding to video generation");

    // Generate keyframe video if we have keyframes
    if (!selectedKeyFrameIndices_.empty() && keyframeEncoder_) {
        LOG_INFO("[KeyFrameAnalyzerService] Starting keyframe video encoding. Keyframes: " +
                 std::to_string(selectedKeyFrameIndices_.size()));

        std::string outputPath = KeyFrameVideoEncoder::generateOutputPath(currentSourceVideoPath_);

        if (keyframeEncoder_->encodeKeyFrames(currentSourceVideoPath_, selectedKeyFrameIndices_,
                                              outputPath)) {
            LOG_INFO("[KeyFrameAnalyzerService] Keyframe video generated: " + outputPath);

            // Notify callback
            std::lock_guard<std::mutex> lock(callbackMutex_);
            if (keyframeVideoCallback_) {
                keyframeVideoCallback_(outputPath);
            }
        } else {
            LOG_ERROR("[KeyFrameAnalyzerService] Failed to generate keyframe video");
        }
    } else {
        LOG_WARN("[KeyFrameAnalyzerService] No keyframes selected, skipping video generation");
    }
}

void KeyFrameAnalyzerService::analysisLoop() {
    LOG_INFO("[KeyFrameAnalyzerService] Analysis loop started");

    while (running_ || !frameQueue_->empty()) {
        FrameItem item;
        if (frameQueue_->pop(item, std::chrono::milliseconds(100))) {
            if (item.resource == nullptr) {
                // EOS Marker
                FrameScore eosScore;
                eosScore.frameIndex = item.context.frameIndex;  // This contains lastFrameId
                eosScore.isEOS = true;
                if (!scoreQueue_->push(eosScore, std::chrono::milliseconds(1000))) {
                    LOG_ERROR("[KeyFrameAnalyzerService] Failed to push EOS to scoreQueue");
                }
                continue;
            }

            auto scores = analyzer_->analyzeFrame(item.resource, item.context);
            auto finalScore = scorer_->score(scores, item.context);

            if (!scoreQueue_->push(finalScore, std::chrono::milliseconds(100))) {
                LOG_WARN("[KeyFrameAnalyzerService] Score queue full, dropping " +
                         std::to_string(finalScore.frameIndex));
            }
        }
    }
}

void KeyFrameAnalyzerService::selectLoop() {
    LOG_INFO("[KeyFrameAnalyzerService] Select loop started");

    std::vector<FrameScore> scoreBuffer;
    const size_t WINDOW_SIZE = 30;

    while (running_ || !scoreQueue_->empty()) {
        FrameScore score;
        if (scoreQueue_->pop(score, std::chrono::milliseconds(100))) {
            if (score.isEOS) {
                // Flush buffer
                if (!scoreBuffer.empty()) {
                    const auto& detConfig = config_.keyframeDetector;
                    int dynamicK = -1;

                    if (!detConfig.useThresholdMode) {
                        dynamicK =
                            static_cast<int>(scoreBuffer.size() * detConfig.targetCompressionRatio);
                        dynamicK = std::max(detConfig.minKeyFrameCount,
                                            std::min(detConfig.maxKeyFrameCount, dynamicK));
                    }

                    auto selectionResult =
                        keyframeDetector_->selectFromFrames(scoreBuffer, dynamicK);
                    for (const auto& selectedScore : selectionResult.KeyframeScores) {
                        selectedFrameQueue_->push(selectedScore, std::chrono::milliseconds(100));
                        if (isOfflineMode_) {
                            selectedKeyFrameIndices_.push_back(selectedScore.frameIndex);
                        }
                    }
                    scoreBuffer.clear();
                }

                // Pass EOS
                selectedFrameQueue_->push(score, std::chrono::milliseconds(1000));
                continue;
            }

            scoreBuffer.push_back(score);

            if (scoreBuffer.size() >= WINDOW_SIZE) {
                const auto& detConfig = config_.keyframeDetector;
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
                        LOG_WARN("[KeyFrameAnalyzerService] Selected queue full, dropping " +
                                 std::to_string(selectedScore.frameIndex));
                    } else {
                        // Track keyframe index for offline video encoding
                        if (isOfflineMode_) {
                            selectedKeyFrameIndices_.push_back(selectedScore.frameIndex);
                        }
                    }
                }

                LOG_DEBUG("[KeyFrameAnalyzerService] Selected: " +
                          std::to_string(selectionResult.selectedFrames) + " / " +
                          std::to_string(selectionResult.totalFrames));

                scoreBuffer.clear();
            }
        }
    }
    // Cleanup of remaining buffer is handled by EOS logic above or here if stopped abruptly
}

void KeyFrameAnalyzerService::publishLoop() {
    LOG_INFO("[KeyFrameAnalyzerService] Publish loop started");

    while (running_ || !selectedFrameQueue_->empty()) {
        FrameScore score;
        if (selectedFrameQueue_->pop(score, std::chrono::milliseconds(100))) {
            if (score.isEOS) {
                // 离线模式：直接通知EOS接收，无需ZMQ发布
                if (isOfflineMode_) {
                    LOG_INFO("[KeyFrameAnalyzerService] Offline mode: EOS received in publishLoop");
                    {
                        std::lock_guard<std::mutex> lock(eosMutex_);
                        eosReceived_ = true;
                    }
                    eosCondition_.notify_one();
                    continue;
                }

                // 在线模式：发送ZMQ停止确认
                uint32_t processed = static_cast<uint32_t>(context_.totalFramesAnalyzed);
                if (publisher_) {
                    publisher_->sendStopAck(processed);
                }

                // 通知 fileReadLoop EOS已处理完成
                {
                    std::lock_guard<std::mutex> lock(eosMutex_);
                    eosReceived_ = true;
                }
                eosCondition_.notify_one();
                continue;
            }

            // 离线模式：跳过元数据发布
            if (isOfflineMode_) {
                continue;
            }

            // 在线模式：发布关键帧元数据
            Protocol::KeyFrameMetaDataMessage meta;
            meta.header.frameID = score.frameIndex;
            meta.header.timestamp = static_cast<uint64_t>(score.timestamp * 1000.0);
            meta.header.Final_Score = score.finalscore;
            meta.header.Sence_Score = score.sceneContribution;
            meta.header.Motion_Score = score.motionContribution;
            meta.header.Text_Score = score.textContribution;
            meta.header.is_Scene_Change = score.rawScores.sceneChangeResult.isSceneChange ? 1 : 0;

            meta.crc32 = Protocol::calculateCrc32(&meta.header, sizeof(meta.header));

            if (publisher_) {
                publisher_->publish(meta);
            }
        }
    }

    LOG_INFO("[KeyFrameAnalyzerService] Publish loop finished");
}

}  // namespace KeyFrame
