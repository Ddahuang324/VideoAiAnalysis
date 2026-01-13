
#include "FrameEncoder.h"

#include <chrono>
#include <cstdint>
#include <exception>
#include <memory>
#include <string>
#include <thread>
#include <utility>

#include "FFmpegWrapper.h"
#include "Log.h"
#include "ThreadSafetyQueue.h"
#include "VideoGrabber.h"

namespace {

constexpr auto POP_TIMEOUT = std::chrono::milliseconds(1000);
constexpr int PROGRESS_NOTIFY_INTERVAL = 30;  // Notify every 30 frames

}  // namespace

FrameEncoder::FrameEncoder(std::shared_ptr<ThreadSafetyQueue<FrameData>> queue,
                           std::shared_ptr<FFmpegWrapper> encoder, const EncoderConfig& config)
    : queue_(std::move(queue)), encoder_(std::move(encoder)), config_(config) {
    LOG_INFO("FrameEncoder constructed with output path: " + config_.video.outputFilePath);
}

FrameEncoder::~FrameEncoder() {
    if (isRunning_.load()) {
        stop();
    }
    LOG_INFO("FrameEncoder destructed");
}

void FrameEncoder::start() {
    if (isRunning_.load()) {
        LOG_WARN("FrameEncoder is already running");
        return;
    }

    isRunning_.store(true);
    encodedFrameCount_.store(0);

    thread_ = std::make_unique<std::thread>(&FrameEncoder::encodeLoop, this);

    LOG_INFO("FrameEncoder started");
}

void FrameEncoder::stop() {
    if (!isRunning_.load()) {
        LOG_WARN("FrameEncoder is not running");
        return;
    }

    isRunning_.store(false);

    if (queue_) {
        queue_->stop();
    }

    if (thread_ && thread_->joinable()) {
        thread_->join();
        thread_.reset();
    }

    if (encoder_) {
        encoder_->finalize();
    }

    LOG_INFO("FrameEncoder stopped. Total encoded frames: " +
             std::to_string(encodedFrameCount_.load()));
}

int64_t FrameEncoder::getOutputFileSize() const {
    if (encoder_) {
        return encoder_->getOutputFileSize();
    }
    return 0;
}

void FrameEncoder::encodeLoop() {
    LOG_INFO("FrameEncoder encode loop started");

    while (isRunning_.load()) {
        FrameData frameData;

        if (!queue_->pop(frameData, POP_TIMEOUT)) {
            if (!isRunning_.load()) {
                break;
            }
            continue;
        }

        if (processFrame(frameData)) {
            encodedFrameCount_++;

            if (encodedFrameCount_.load() % PROGRESS_NOTIFY_INTERVAL == 0) {
                notifyProgress();
            }
        } else {
            notifyError("Failed to encode frame at index " +
                        std::to_string(encodedFrameCount_.load()));
        }
    }

    notifyFinished();

    LOG_INFO("FrameEncoder encode loop finished. Total frames encoded: " +
             std::to_string(encodedFrameCount_.load()));
}

bool FrameEncoder::processFrame(const FrameData& frameData) {
    if (!encoder_ || !encoder_->isInitialized()) {
        LOG_ERROR("Encoder is not initialized");
        return false;
    }

    if (!encoder_->encoderFrame(frameData)) {
        LOG_ERROR("FFmpegWrapper failed to encode frame: " + encoder_->getLastError());
        return false;
    }

    return true;
}

void FrameEncoder::notifyProgress() {
    if (!progressCallback_) {
        return;
    }

    try {
        progressCallback_(encodedFrameCount_.load(), getOutputFileSize());
    } catch (const std::exception& e) {
        LOG_ERROR("Exception in progress callback: " + std::string(e.what()));
    }
}

void FrameEncoder::notifyFinished() {
    if (!finishedCallback_) {
        return;
    }

    try {
        finishedCallback_(encodedFrameCount_.load(), config_.video.outputFilePath);
    } catch (const std::exception& e) {
        LOG_ERROR("Exception in finished callback: " + std::string(e.what()));
    }
}

void FrameEncoder::notifyError(const std::string& errorMessage) {
    LOG_ERROR("FrameEncoder error: " + errorMessage);

    if (!errorCallback_) {
        return;
    }

    try {
        errorCallback_(errorMessage);
    } catch (const std::exception& e) {
        LOG_ERROR("Exception in error callback: " + std::string(e.what()));
    }
}
