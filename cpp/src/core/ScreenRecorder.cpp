#include "ScreenRecorder.h"

#include <chrono>
#include <cstdint>
#include <memory>
#include <string>

#include "AudioData.h"
#include "AudioEncoder.h"
#include "AudioGrabber.h"
#include "FFmpegWrapper.h"
#include "FrameEncoder.h"
#include "FrameGrabberThread.h"
#include "Log.h"
#include "ThreadSafetyQueue.h"
#include "VideoGrabber.h"
#include "VideoGrabberFactory.h"
#include "WasapiAudioGrabber.h"


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

    // 根据模式选择采集器
    GrabberType preferredType =
        (m_mode == RecorderMode::SNAPSHOT) ? GrabberType::GDI : GrabberType::AUTO;
    m_grabber_ = VideoGrabberFactory::createGrabber(preferredType);

    if (!m_grabber_) {
        m_lastError = "Failed to create screen grabber";
        LOG_ERROR(m_lastError);
        m_isRecording.store(false);
        return false;
    }

    m_frameQueue_ = std::make_shared<ThreadSafetyQueue<FrameData>>(30);

    // 在获取配置前先启动 Grabber，以确保能够获取到正确的分辨率和 FPS
    if (!m_grabber_->start()) {
        m_lastError = "Failed to start screen grabber";
        LOG_ERROR(m_lastError);
        m_isRecording.store(false);
        return false;
    }

    // 根据模式设定目标帧率
    int target_fps = (m_mode == RecorderMode::SNAPSHOT) ? 1 : m_grabber_->getFps();
    if (target_fps <= 0)
        target_fps = 30;  // 后备方案

    grabber_thread_ = std::make_shared<FrameGrabberThread>(m_grabber_, *m_frameQueue_, target_fps);

    grabber_thread_->setProgressCallback([](int64_t frames, int queue_size, double fps) {
        LOG_INFO("Grabber progress - Frames: " + std::to_string(frames) +
                 ", Queue Size: " + std::to_string(queue_size) + ", FPS: " + std::to_string(fps));
    });

    grabber_thread_->setErrorCallback([this](const std::string& errorMessage) {
        m_lastError = errorMessage;
        LOG_ERROR("Grabber error: " + errorMessage);
        if (m_errorCallback) {
            m_errorCallback(errorMessage);
        }
    });

    EncoderConfig config = encoderConfigFromGrabber(m_grabber_.get());
    if (m_mode == RecorderMode::SNAPSHOT) {
        config.fps = 1;
    }
    config.outputFilePath = path;

    // 初始化 FFmpegWrapper
    m_ffmpegWrapper_ = std::make_shared<FFmpegWrapper>();
    if (!m_ffmpegWrapper_->initialize(config)) {
        m_lastError = "Failed to initialize FFmpeg: " + m_ffmpegWrapper_->getLastError();
        LOG_ERROR(m_lastError);
        m_isRecording.store(false);
        return false;
    }

    // 视频编码器
    m_encoder_ = std::make_unique<FrameEncoder>(m_frameQueue_, m_ffmpegWrapper_, config);

    // 音频采集与编码 (如果启用)
    if (config.enableAudio) {
        m_audioQueue_ = std::make_shared<ThreadSafetyQueue<AudioData>>(100);
        m_audioGrabber_ = std::make_shared<WasapiAudioGrabber>();
        m_audioGrabber_->setCallback([this](const AudioData& data) {
            if (m_audioQueue_) {
                m_audioQueue_->push(data, std::chrono::milliseconds(100));
            }
        });

        if (m_audioGrabber_->start()) {
            m_audioEncoder_ = std::make_unique<AudioEncoder>(m_audioQueue_, m_ffmpegWrapper_);
            m_audioEncoder_->start();
        } else {
            LOG_WARN("Failed to start audio grabber, recording video only");
        }
    }

    m_encoder_->setProgressCallback([this](int64_t frames, int64_t size) {
        LOG_INFO("Encoder progress - Frames: " + std::to_string(frames) +
                 ", Size: " + std::to_string(size));
        if (m_progressCallback) {
            m_progressCallback(frames, size);
        }
    });

    m_encoder_->setErrorCallback([this](const std::string& errorMessage) {
        m_lastError = errorMessage;
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

    if (m_audioGrabber_) {
        m_audioGrabber_->stop();
        m_audioGrabber_.reset();
    }

    if (m_encoder_) {
        m_encoder_->stop();
        m_encoder_.reset();
    }

    if (m_audioEncoder_) {
        m_audioEncoder_->stop();
        m_audioEncoder_.reset();
    }

    if (m_ffmpegWrapper_) {
        m_ffmpegWrapper_->finalize();
        m_ffmpegWrapper_.reset();
    }

    m_frameQueue_.reset();
    m_audioQueue_.reset();
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
    if (m_ffmpegWrapper_) {
        return m_ffmpegWrapper_->getOutputFileSize();
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
void ScreenRecorder::setRecorderMode(RecorderMode mode) {
    m_mode = mode;
}

RecorderMode ScreenRecorder::getRecorderMode() const {
    return m_mode;
}
