#include "ScreenRecorder.h"

#include <cstdint>
#include <memory>
#include <string>

#include "FFmpegWrapper.h"
#include "FrameEncoder.h"
#include "FrameGrabberThread.h"
#include "GrabberFactory.h"
#include "IScreenGrabber.h"
#include "Log.h"
#include "ThreadSafetyQueue.h"

ScreenRecorder::ScreenRecorder() = default;

ScreenRecorder::~ScreenRecorder() {
    if (m_isRecording.load()) {
        stopRecording();
    }
}

bool ScreenRecorder::startRecording(std::string& path) {
    if (m_isRecording.load()) {
        return false;  // 已经在录制中
    }
    // TODO: 实现录制逻辑
    m_isRecording.store(true);

    m_grabber_ = GrabberFactory::createGrabber(GrabberType::AUTO);

    if (!m_grabber_) {
        LastError = "Failed to create screen grabber";
        LOG_ERROR(LastError);
        m_isRecording.store(false);
        return false;
    }

    m_frameQueue_ = std::make_shared<ThreadSafetyQueue<FrameData>>(30);

    // 在获取配置前先启动 Grabber，以确保能够获取到正确的分辨率和 FPS
    if (!m_grabber_->start()) {
        LastError = "Failed to start screen grabber";
        LOG_ERROR(LastError);
        m_isRecording.store(false);
        return false;
    }

    grabber_thread_ =
        std::make_shared<FrameGrabberThread>(m_grabber_, *m_frameQueue_, m_grabber_->getFps());

    grabber_thread_->setProgressCallback([this](int64_t frames, int queue_size, double fps) {
        LOG_INFO("Grabber progress - Frames: " + std::to_string(frames) +
                 ", Queue Size: " + std::to_string(queue_size) + ", FPS: " + std::to_string(fps));
    });

    grabber_thread_->setErrorCallback([this](const std::string& errorMessage) {
        LastError = errorMessage;
        LOG_ERROR("Grabber error: " + errorMessage);
        if (m_errorCallback) {
            m_errorCallback(errorMessage);
        }
    });

    EncoderConfig config = encoderConfigFromGrabber(m_grabber_.get());
    config.outputFilePath = path;
    m_encoder_ = std::make_unique<FrameEncoder>(m_frameQueue_, config);

    m_encoder_->setProgressCallback([this](int64_t frames, int64_t size) {
        LOG_INFO("Encoder progress - Frames: " + std::to_string(frames) +
                 ", Size: " + std::to_string(size));
        if (m_progressCallback) {
            m_progressCallback(frames, size);
        }
    });

    m_encoder_->setErrorCallback([this](const std::string& errorMessage) {
        LastError = errorMessage;
        LOG_ERROR("Encoder error: " + errorMessage);
        if (m_errorCallback) {
            m_errorCallback(errorMessage);
        }
    });

    grabber_thread_->start();

    m_encoder_->start();

    return true;
}

void ScreenRecorder::stopRecording() {
    if (!m_isRecording.load()) {
        return;  // 不在录制中
    }

    LOG_INFO("[ScreenRecorder] Stopping Recording");
    if (grabber_thread_) {
        grabber_thread_->stop();
        grabber_thread_.reset();
    }
    if (m_encoder_) {
        m_encoder_->stop();
        m_encoder_.reset();
    }

    m_frameQueue_.reset();
    m_grabber_.reset();

    m_isRecording.store(false);
    LOG_INFO("[ScreenRecorder] Recording Stopped");
}

void ScreenRecorder::pauseRecording() {
    if (grabber_thread_) {
        grabber_thread_->pause();
    }
}

void ScreenRecorder::resumeRecording() {
    if (grabber_thread_) {
        grabber_thread_->resume();
    }
}

int64_t ScreenRecorder::getFrameCount() const {
    if (grabber_thread_) {
        return grabber_thread_->getCapturedFrameCount();
    }
    return 0;
}

int64_t ScreenRecorder::getEncodedCount() const {
    if (m_encoder_) {
        return m_encoder_->getEncodedFrameCount();
    }
    return 0;
}

int64_t ScreenRecorder::getDroppedCount() const {
    if (grabber_thread_) {
        return grabber_thread_->getDroppedFrameCount();
    }
    return 0;
}

int64_t ScreenRecorder::getOutputFileSize() const {
    if (m_encoder_) {
        return m_encoder_->getOutputFileSize();
    }
    return 0;
}

double ScreenRecorder::getCurrentFps() const {
    if (grabber_thread_) {
        return grabber_thread_->getCurrentFps();
    }
    return 0.0;
}

bool ScreenRecorder::is_Recording() const {
    return m_isRecording.load();
}

void ScreenRecorder::setProgressCallback(ProgressCallback callback) {
    m_progressCallback = callback;
}

void ScreenRecorder::setErrorCallback(ErrorCallback callback) {
    m_errorCallback = callback;
}