
#include "FrameEncoder.h"

#include <chrono>
#include <cstdint>
#include <exception>
#include <memory>
#include <string>
#include <thread>

#include "FFmpegWrapper.h"
#include "IScreenGrabber.h"
#include "Log.h"
#include "ThreadSafetyQueue.h"

FrameEncoder::FrameEncoder(std::shared_ptr<ThreadSafetyQueue<FrameData>> queue,
                           const EncoderConfig& config)
    : queue_(queue), encoder_(std::make_unique<FFmpegWrapper>()), config_(config) {
    LOG_INFO("FrameEncoder constructed with output path: " + config.outputFilePath);
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

    // 初始化编码器
    if (!encoder_->initialize(config_)) {
        notifyError("Failed to initialize encoder: " + encoder_->getLastError());
        return;
    }

    isRunning_.store(true);
    encodedFrameCount_.store(0);

    // 启动编码线程
    thread_ = std::make_unique<std::thread>([this]() { encodeLoop(); });

    LOG_INFO("FrameEncoder started");
}

void FrameEncoder::stop() {
    if (!isRunning_.load()) {
        LOG_WARN("FrameEncoder is not running");
        return;
    }

    isRunning_.store(false);

    // 通知队列停止，避免线程永久等待
    if (queue_) {
        queue_->stop();
    }

    // 等待编码线程完成
    if (thread_ && thread_->joinable()) {
        thread_->join();
        thread_.reset();
    }

    // 停止编码器
    encoder_->finalize();

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

        // 从队列中获取帧数据，超时时间为1秒
        if (!queue_->pop(frameData, std::chrono::milliseconds(1000))) {
            // 超时或队列停止，检查是否应该继续
            if (!isRunning_.load()) {
                break;
            }
            continue;  // 继续等待
        }

        // 处理帧数据
        if (ProcessFrame(frameData)) {
            encodedFrameCount_++;

            // 每处理N帧或特定时间间隔通知一次进度
            if (encodedFrameCount_.load() % 30 == 0) {  // 每30帧通知一次
                notifyProgress();
            }
        } else {
            notifyError("Failed to encode frame at index " +
                        std::to_string(encodedFrameCount_.load()));
        }
    }

    // 编码完成，通知完成
    notifyFinished();

    LOG_INFO("FrameEncoder encode loop finished. Total frames encoded: " +
             std::to_string(encodedFrameCount_.load()));
}

bool FrameEncoder::ProcessFrame(const FrameData& frameData) {
    if (!encoder_ || !encoder_->isInitialized()) {
        LOG_ERROR("Encoder is not initialized");
        return false;
    }

    // 调用FFmpegWrapper的编码接口
    if (!encoder_->encoderFrame(frameData)) {
        LOG_ERROR("FFmpegWrapper failed to encode frame: " + encoder_->getLastError());
        return false;
    }

    return true;
}

void FrameEncoder::notifyProgress() {
    if (progressCallback_) {
        try {
            progressCallback_(encodedFrameCount_.load(), getOutputFileSize());
        } catch (const std::exception& e) {
            LOG_ERROR("Exception in progress callback: " + std::string(e.what()));
        }
    }
}

void FrameEncoder::notifyFinished() {
    if (finishedCallback_) {
        try {
            finishedCallback_(encodedFrameCount_.load(), config_.outputFilePath);
        } catch (const std::exception& e) {
            LOG_ERROR("Exception in finished callback: " + std::string(e.what()));
        }
    }
}

void FrameEncoder::notifyError(const std::string& errorMessage) {
    LOG_ERROR("FrameEncoder error: " + errorMessage);

    if (errorCallback_) {
        try {
            errorCallback_(errorMessage);
        } catch (const std::exception& e) {
            LOG_ERROR("Exception in error callback: " + std::string(e.what()));
        }
    }
}
