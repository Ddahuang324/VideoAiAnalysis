#include "Process/Analyzer/AnalyzerAPI.h"

#include <cstdint>
#include <exception>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include "Infra/Log.h"
#include "core/KeyFrame/KeyFrameAnalyzerService.h"

namespace Analyzer {

class AnalyzerAPI::Impl {
public:
    std::unique_ptr<KeyFrame::KeyFrameAnalyzerService> service_;
    AnalysisStatus status_ = AnalysisStatus::IDLE;
    AnalysisStats stats_;
    std::string lastError_;
    mutable std::mutex mutex_;

    // 回调函数
    StatusCallback statusCallback_;
    KeyFrameCallback keyFrameCallback_;

    void updateStatus(AnalysisStatus s) {
        std::lock_guard<std::mutex> lock(mutex_);
        status_ = s;

        // 触发状态回调
        if (statusCallback_) {
            statusCallback_(s);
        }
    }
};

AnalyzerAPI::AnalyzerAPI() : impl_(std::make_unique<Impl>()) {}

AnalyzerAPI::~AnalyzerAPI() {
    shutdown();
}

bool AnalyzerAPI::initialize(const AnalyzerConfig& config) {
    LOG_INFO("Initializing AnalyzerAPI...");

    // 转换配置
    KeyFrame::KeyFrameAnalyzerService::Config serviceConfig;
    serviceConfig.zmq.frameSubEndpoint = config.zmqSubscribeEndpoint;
    serviceConfig.zmq.keyframePubEndpoint = config.zmqPublishEndpoint;
    serviceConfig.enableTextRecognition = config.enableTextRecognition;

    serviceConfig.models.sceneModelPath = config.sceneModelPath;
    serviceConfig.models.motionModelPath = config.motionModelPath;
    serviceConfig.models.textDetModelPath = config.textDetModelPath;
    serviceConfig.models.textRecModelPath = config.textRecModelPath;

    try {
        impl_->service_ = std::make_unique<KeyFrame::KeyFrameAnalyzerService>(serviceConfig);
        impl_->updateStatus(AnalysisStatus::IDLE);
        return true;
    } catch (const std::exception& e) {
        impl_->lastError_ = std::string("Initialization failed: ") + e.what();
        LOG_ERROR(impl_->lastError_);
        impl_->updateStatus(AnalysisStatus::ERROR);
        return false;
    }
}

bool AnalyzerAPI::start() {
    if (!impl_->service_) {
        LOG_ERROR("Cannot start Analyzer: Not initialized");
        return false;
    }

    impl_->updateStatus(AnalysisStatus::RUNNING);
    LOG_INFO("Starting Analyzer (async)...");
    if (!impl_->service_->start()) {
        impl_->updateStatus(AnalysisStatus::ERROR);
        impl_->lastError_ = "Failed to start analyzer service";
        LOG_ERROR(impl_->lastError_);
        return false;
    }
    return true;
}

bool AnalyzerAPI::stop() {
    if (!impl_->service_)
        return true;

    LOG_INFO("Stopping Analyzer...");
    impl_->updateStatus(AnalysisStatus::STOPPING);
    impl_->service_->stop();
    impl_->updateStatus(AnalysisStatus::IDLE);
    return true;
}

void AnalyzerAPI::shutdown() {
    stop();
    impl_->service_.reset();
}

AnalysisStatus AnalyzerAPI::getStatus() const {
    std::lock_guard<std::mutex> lock(impl_->mutex_);
    if (impl_->status_ == AnalysisStatus::RUNNING && impl_->service_ &&
        !impl_->service_->isRunning()) {
        return AnalysisStatus::IDLE;
    }
    return impl_->status_;
}

AnalysisStats AnalyzerAPI::getStats() const {
    std::lock_guard<std::mutex> lock(impl_->mutex_);
    if (impl_->service_) {
        auto ctx = impl_->service_->getContext();
        impl_->stats_.analyzedFrameCount = static_cast<int64_t>(ctx.totalFramesAnalyzed);

        auto latestKFs = impl_->service_->getLatestKeyFrames();
        impl_->stats_.latestKeyFrames.clear();
        for (const auto& kf : latestKFs) {
            impl_->stats_.latestKeyFrames.push_back(
                {static_cast<int64_t>(kf.frameIndex), kf.finalscore, kf.timestamp});
        }
        impl_->stats_.keyframeCount = impl_->service_->getTotalKeyFramesCount();

        const auto& config = impl_->service_->getConfig();
        impl_->stats_.activeConfig.textRecognitionEnabled = config.enableTextRecognition;
        impl_->stats_.activeConfig.threadCount = config.pipeline.analysisThreadCount;

        std::string models = "Scene+Motion";
        if (config.enableTextRecognition)
            models += "+OCR";
        impl_->stats_.activeConfig.activeModelInfo = models;
    }
    return impl_->stats_;
}

std::string AnalyzerAPI::getLastError() const {
    std::lock_guard<std::mutex> lock(impl_->mutex_);
    if (impl_->service_) {
        auto err = impl_->service_->getLastError();
        if (!err.empty())
            return err;
    }
    return impl_->lastError_;
}

void AnalyzerAPI::setStatusCallback(StatusCallback callback) {
    std::lock_guard<std::mutex> lock(impl_->mutex_);
    impl_->statusCallback_ = callback;
}

void AnalyzerAPI::setKeyFrameCallback(KeyFrameCallback callback) {
    std::lock_guard<std::mutex> lock(impl_->mutex_);
    impl_->keyFrameCallback_ = callback;

    // 注意: KeyFrameAnalyzerService 目前没有提供回调接口
    // 回调函数已保存,可在 getStats() 中主动轮询最新关键帧来触发
}

}  // namespace Analyzer
