#include "ScreenRecorder.h"

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <utility>

#include "AudioData.h"
#include "AudioEncoder.h"
#include "AudioGrabber.h"
#include "FFmpegWrapper.h"
#include "FrameEncoder.h"
#include "FrameGrabberThread.h"
#include "FramePublisher.h"
#include "KeyFrameMetaDataSubscriber.h"
#include "Log.h"
#include "Protocol.h"
#include "RingFrameBuffer.h"
#include "ThreadSafetyQueue.h"
#include "VideoGrabber.h"
#include "VideoGrabberFactory.h"
#include "WasapiAudioGrabber.h"

ScreenRecorder::ScreenRecorder() = default;

ScreenRecorder::~ScreenRecorder() {
    // 确保所有辅助线程都已停止，防止在对象析构时发生 std::terminate 或访问已释放资源
    stopPublishing();
    stopKeyFrameMetaDataReceiving();
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
    m_videoRingBuffer_ = std::make_unique<RingFrameBuffer>(300);  // 缓存最近300帧(约10秒)

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

    grabber_thread_->setFrameCallback(
        [this](const FrameData& data) { this->pushToPublishQueue(data); });

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
            if (m_keyFrameAudioQueue_) {
                // 使用较短超时以不阻塞主录像采集
                m_keyFrameAudioQueue_->push(data, std::chrono::milliseconds(5));
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

    // 1. 先停止依赖外部资源的异步线程，防止竞态条件（如主循环还在写入 RingBuffer 或发送 ZMQ 消息）
    stopPublishing();
    stopKeyFrameMetaDataReceiving();
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
    m_videoRingBuffer_.reset();
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

//================发布线程================//
bool ScreenRecorder::startPublishing() {
    if (publishrunning_)
        return false;
    publishrunning_ = true;
    m_publishingThread = std::thread(&ScreenRecorder::publishingLoop, this);
    return true;
}

void ScreenRecorder::stopPublishing() {
    publishrunning_ = false;
    m_publishCondVar.notify_all();
    if (m_publishingThread.joinable()) {
        m_publishingThread.join();
    }
}

void ScreenRecorder::pushToPublishQueue(const FrameData& data) {
    {
        std::lock_guard<std::mutex> lock(m_publishMutex);
        if (m_publishqueue.size() >= 30) {
            m_publishqueue.pop();
        }
        m_publishqueue.push(data);  // 浅拷贝 FrameData，包含共享指针和Mat引用
    }
    m_publishCondVar.notify_one();
}

void ScreenRecorder::publishingLoop() {
    m_framePublisher = std::make_unique<MQInfra::FramePublisher>();

    if (!m_framePublisher->initialize("tcp://*:5555")) {
        LOG_ERROR("Failed to initialize FramePublisher.");
        return;
    }

    LOG_INFO("Publish thread Started");

    while (publishrunning_) {
        FrameData frame;
        {
            std::unique_lock<std::mutex> lock(m_publishMutex);

            if (!m_publishCondVar.wait_for(lock, std::chrono::milliseconds(100), [this] {
                    return !m_publishqueue.empty() || !publishrunning_;
                })) {
                continue;  // 超时，继续等待
            }

            if (!publishrunning_)
                break;

            frame = std::move(m_publishqueue.front());
            m_publishqueue.pop();
        }

        // 同时推送到环形缓冲区，供关键帧编码使用
        if (m_videoRingBuffer_) {
            m_videoRingBuffer_->push(frame.frame_ID, frame.frame, frame.timestamp_ms);
        }

        Protocol::FrameHeader msgheader;
        msgheader.magic_num = Protocol::FRAME_MAGIC;
        msgheader.version = 1;
        msgheader.message_type = 0x01;
        msgheader.frameID = frame.frame_ID;
        msgheader.timestamp = frame.timestamp_ms;
        msgheader.width = frame.frame.cols;
        msgheader.height = frame.frame.rows;
        msgheader.channels = frame.frame.channels();
        // 直接使用 Mat 的数据大小，无需拷贝到 vector
        size_t dataSize = frame.frame.total() * frame.frame.elemSize();
        msgheader.data_size = static_cast<uint32_t>(dataSize);

        // 计算 CRC (直接从 Mat.data 计算)
        uint32_t crc32 = Protocol::calculateCrc32(frame.frame.data, dataSize) ^ 0xFFFFFFFF;

        // 使用零拷贝发布
        if (!m_framePublisher->publishRaw(msgheader, frame.frame.data, dataSize, crc32)) {
            LOG_ERROR("Failed to publish frame ID: " + std::to_string(frame.frame_ID));
        }
    }
    m_framePublisher->shutdown();
    LOG_INFO("Publish Thread Stopped");
}

//================接受线程================//
bool ScreenRecorder::startKeyFrameMetaDataReceiving(const std::string& keyFramePath) {
    if (receiverunning_) {
        return false;  // 已经在接收中
    }

    m_keyFrameOutputPath_ = keyFramePath;

    // 初始化关键帧编码器
    if (m_grabber_) {
        EncoderConfig config = encoderConfigFromGrabber(m_grabber_.get());
        config.outputFilePath = m_keyFrameOutputPath_;
        // 关键帧模式下，通常我们希望高质量，可以根据需要调整 FPS 或码率
        // 这里暂时复用主配置，但路径不同

        m_keyFrameFFmpegWrapper_ = std::make_shared<FFmpegWrapper>();
        if (!m_keyFrameFFmpegWrapper_->initialize(config)) {
            LOG_ERROR("Failed to initialize KeyFrame FFmpegWrapper: " +
                      m_keyFrameFFmpegWrapper_->getLastError());
            return false;
        }

        if (config.enableAudio) {
            m_keyFrameAudioQueue_ = std::make_shared<ThreadSafetyQueue<AudioData>>(100);
            m_keyFrameAudioEncoder_ =
                std::make_unique<AudioEncoder>(m_keyFrameAudioQueue_, m_keyFrameFFmpegWrapper_);
            m_keyFrameAudioEncoder_->start();
        }
    }

    keyFrameMetaDataSubscriber_ = std::make_unique<MQInfra::KeyFrameMetaDataSubscriber>();

    if (!keyFrameMetaDataSubscriber_->initialize("tcp://localhost:5556")) {
        LOG_ERROR("Failed to initialize KeyFrameMetaDataSubscriber.");
        keyFrameMetaDataSubscriber_.reset();
        return false;
    }

    receiverunning_ = true;
    keyFrameReceiveThread_ = std::thread(&ScreenRecorder::keyFrameMetaDataReceiveLoop, this);

    return true;
}

bool ScreenRecorder::stopKeyFrameMetaDataReceiving() {
    if (!keyFrameMetaDataSubscriber_) {
        return false;  // 不在接收中
    }

    receiverunning_ = false;
    keyFrameMetaDataCondVar_.notify_all();
    if (keyFrameReceiveThread_.joinable()) {
        keyFrameReceiveThread_.join();
    }

    if (m_keyFrameAudioEncoder_) {
        m_keyFrameAudioEncoder_->stop();
        m_keyFrameAudioEncoder_.reset();
    }

    if (m_keyFrameFFmpegWrapper_) {
        m_keyFrameFFmpegWrapper_->finalize();
        m_keyFrameFFmpegWrapper_.reset();
    }

    if (m_keyFrameAudioQueue_) {
        m_keyFrameAudioQueue_->stop();
        m_keyFrameAudioQueue_.reset();
    }

    keyFrameMetaDataSubscriber_->shutdown();
    keyFrameMetaDataSubscriber_.reset();

    return true;
}

void ScreenRecorder::keyFrameMetaDataReceiveLoop() {
    LOG_INFO("Key Frame MetaData Receive Thread Started");

    while (receiverunning_) {
        auto msg = keyFrameMetaDataSubscriber_->receiveMetaData(100);

        if (!msg) {
            LOG_WARN("Didnt Received Messages yet");
            continue;  // 超时或无消息，继续等待
        }

        Protocol::KeyFrameMetaDataHeader header = msg->header;

        if (header.magic_num != Protocol::METADATA_MAGIC) {
            LOG_ERROR("Invalid Key Frame MetaData Header received. Expected: " +
                      std::to_string(Protocol::METADATA_MAGIC) +
                      ", Got: " + std::to_string(header.magic_num));
            continue;
        }

        {
            std::lock_guard<std::mutex> lock(keyFrameMetaDataMutex_);
            keyFrameMetaDataQueue_.push(header);
            keyFrameMetaDataCondVar_.notify_one();
        }

        // 独立的关键帧编码处理
        if (m_keyFrameFFmpegWrapper_ && m_videoRingBuffer_) {
            uint64_t timestamp = 0;
            auto frameMat = m_videoRingBuffer_->get(header.frameID, timestamp);
            if (frameMat) {
                FrameData fd;
                fd.frame = *frameMat;
                fd.frame_ID = header.frameID;
                fd.timestamp_ms = timestamp;

                if (!m_keyFrameFFmpegWrapper_->encoderFrame(fd)) {
                    LOG_ERROR("Failed to encode key frame ID: " + std::to_string(header.frameID));
                }
            }
        }

        LOG_INFO("Received Key Frame MetaData - FrameID: " + std::to_string(header.frameID) +
                 ", Final_Score: " + std::to_string(header.Final_Score) +
                 ", Scene_Change: " + std::to_string(header.is_Scene_Change));
    }
    keyFrameMetaDataSubscriber_->shutdown();
    LOG_INFO("Key Frame MetaData Receive Thread Stopped");
}