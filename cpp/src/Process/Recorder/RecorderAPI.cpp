#include "Process/Recorder/RecorderAPI.h"

#include <chrono>
#include <memory>
#include <string>

#include "core/ScreenRecorder/ScreenRecorder.h"

namespace Recorder {

struct RecorderAPI::Impl {
    std::unique_ptr<ScreenRecorder> recorder_;
    RecordingStatus status_ = RecordingStatus::IDLE;
    std::chrono::steady_clock::time_point startTime_;
    std::string lastError_;
    StatusCallBack statusCallback_;
    ErrorCallBack errorCallback_;
};

RecorderAPI::RecorderAPI() : impl_(std::make_unique<Impl>()) {}

RecorderAPI::~RecorderAPI() = default;

bool RecorderAPI::initialize(const RecorderConfig& config) {
    impl_->recorder_ = std::make_unique<ScreenRecorder>();
    // 目前 ScreenRecorder 可能没有统一的 initialize，我们根据需要设置模式等
    // 如果后续 ScreenRecorder 增加了 Config 支持，代码将在这里调用
    impl_->status_ = RecordingStatus::INITIALIZING;

    // 这里简单映射一下状态
    impl_->status_ = RecordingStatus::IDLE;
    return true;
}

bool RecorderAPI::start() {
    if (!impl_->recorder_)
        return false;

    // TODO: 从 config 获取路径或生成路径
    std::string path = "output.mp4";

    if (impl_->recorder_->startRecording(path)) {
        impl_->status_ = RecordingStatus::RECORDING;
        impl_->startTime_ = std::chrono::steady_clock::now();

        // 自动启动发布
        impl_->recorder_->startPublishing();

        // 调用状态回调
        if (impl_->statusCallback_) {
            impl_->statusCallback_(impl_->status_);
        }

        return true;
    } else {
        impl_->lastError_ = impl_->recorder_->getLastError();
        impl_->status_ = RecordingStatus::ERROR;

        // 调用错误回调
        if (impl_->errorCallback_) {
            impl_->errorCallback_(impl_->lastError_);
        }

        return false;
    }
}

bool RecorderAPI::pause() {
    if (impl_->status_ == RecordingStatus::RECORDING) {
        impl_->recorder_->pauseRecording();
        impl_->status_ = RecordingStatus::PAUSED;

        // 调用状态回调
        if (impl_->statusCallback_) {
            impl_->statusCallback_(impl_->status_);
        }

        return true;
    }
    return false;
}

bool RecorderAPI::resume() {
    if (impl_->status_ == RecordingStatus::PAUSED) {
        impl_->recorder_->resumeRecording();
        impl_->status_ = RecordingStatus::RECORDING;

        // 调用状态回调
        if (impl_->statusCallback_) {
            impl_->statusCallback_(impl_->status_);
        }

        return true;
    }
    return false;
}

bool RecorderAPI::stop() {
    if (impl_->recorder_) {
        impl_->recorder_->stopRecording();
        impl_->status_ = RecordingStatus::STOPPING;

        // 调用状态回调
        if (impl_->statusCallback_) {
            impl_->statusCallback_(RecordingStatus::STOPPING);
        }

        impl_->status_ = RecordingStatus::IDLE;

        // 调用状态回调
        if (impl_->statusCallback_) {
            impl_->statusCallback_(impl_->status_);
        }
    }
    return true;
}

void RecorderAPI::shutdown() {
    stop();
    impl_->recorder_.reset();
}

RecordingStatus RecorderAPI::getStatus() const {
    return impl_->status_;
}

RecordingStats RecorderAPI::getStats() const {
    RecordingStats stats{};
    if (impl_->recorder_) {
        stats.frame_count = impl_->recorder_->getFrameCount();
        stats.encoded_count = impl_->recorder_->getEncodedCount();
        stats.dropped_count = impl_->recorder_->getDroppedCount();
        stats.file_size_bytes = impl_->recorder_->getOutputFileSize();
        stats.current_fps = impl_->recorder_->getCurrentFps();
        auto now = std::chrono::steady_clock::now();
        std::chrono::duration<double> elapsed = now - impl_->startTime_;
        stats.duration_seconds = elapsed.count();
    }
    return stats;
}

std::string RecorderAPI::getLastError() const {
    return impl_->lastError_;
}

void RecorderAPI::setStatusCallback(StatusCallBack callback) {
    impl_->statusCallback_ = callback;
}

void RecorderAPI::setErrorCallback(ErrorCallBack callback) {
    impl_->errorCallback_ = callback;
}

}  // namespace Recorder
