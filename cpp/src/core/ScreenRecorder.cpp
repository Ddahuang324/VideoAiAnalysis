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
    if (isRecording_.load()) {
        stopRecording();
    }
}

bool ScreenRecorder::startRecording(std::string& path) {
    if (isRecording_.load()) {
        return false;  // 已经在录制中
    }
    // TODO: 实现录制逻辑
    isRecording_.store(true);

    // 根据模式选择采集器
    GrabberType preferredType =
        (mode_ == RecorderMode::SNAPSHOT) ? GrabberType::GDI : GrabberType::AUTO;
    grabber_ = VideoGrabberFactory::createGrabber(preferredType);

    if (!grabber_) {
        lastError_ = "Failed to create screen grabber";
        LOG_ERROR(lastError_);
        isRecording_.store(false);
        return false;
    }

    frameQueue_ = std::make_shared<ThreadSafetyQueue<FrameData>>(30);
    videoRingBuffer_ = std::make_unique<RingFrameBuffer>(300);  // 缓存最近300帧(约10秒)

    // 在获取配置前先启动 Grabber，以确保能够获取到正确的分辨率和 FPS
    if (!grabber_->start()) {
        lastError_ = "Failed to start screen grabber";
        LOG_ERROR(lastError_);
        isRecording_.store(false);
        return false;
    }

    // 根据模式设定目标帧率
    int target_fps = (mode_ == RecorderMode::SNAPSHOT) ? 1 : grabber_->getFps();
    if (target_fps <= 0)
        target_fps = 30;  // 后备方案

    grabberThread_ = std::make_shared<FrameGrabberThread>(grabber_, *frameQueue_, target_fps);

    grabberThread_->setFrameCallback(
        [this](const FrameData& data) { this->pushToPublishQueue(data); });

    grabberThread_->setProgressCallback([](int64_t frames, int queue_size, double fps) {
        LOG_INFO("Grabber progress - Frames: " + std::to_string(frames) +
                 ", Queue Size: " + std::to_string(queue_size) + ", FPS: " + std::to_string(fps));
    });

    grabberThread_->setErrorCallback([this](const std::string& errorMessage) {
        lastError_ = errorMessage;
        LOG_ERROR("Grabber error: " + errorMessage);
        if (errorCallback_) {
            errorCallback_(errorMessage);
        }
    });

    EncoderConfig config = encoderConfigFromGrabber(grabber_.get());
    if (mode_ == RecorderMode::SNAPSHOT) {
        config.video.fps = 1;
    }
    config.video.outputFilePath = path;

    // 初始化 FFmpegWrapper
    ffmpegWrapper_ = std::make_shared<FFmpegWrapper>();
    if (!ffmpegWrapper_->initialize(config)) {
        lastError_ = "Failed to initialize FFmpeg: " + ffmpegWrapper_->getLastError();
        LOG_ERROR(lastError_);
        isRecording_.store(false);
        return false;
    }

    // 视频编码器
    encoder_ = std::make_unique<FrameEncoder>(frameQueue_, ffmpegWrapper_, config);

    // 音频采集与编码 (如果启用)
    if (config.audio.enabled) {
        audioQueue_ = std::make_shared<ThreadSafetyQueue<AudioData>>(100);

        audioGrabber_ = std::make_shared<WasapiAudioGrabber>();
        audioGrabber_->setCallback([this](const AudioData& data) {
            if (audioQueue_) {
                audioQueue_->push(data, std::chrono::milliseconds(100));
            }
            if (keyFrameAudioQueue_) {
                // 使用较短超时以不阻塞主录像采集
                keyFrameAudioQueue_->push(data, std::chrono::milliseconds(5));
            }
        });

        if (audioGrabber_->start()) {
            audioEncoder_ = std::make_unique<AudioEncoder>(audioQueue_, ffmpegWrapper_);
            audioEncoder_->start();
        } else {
            LOG_WARN("Failed to start audio grabber, recording video only");
        }
    }

    encoder_->setProgressCallback([this](int64_t frames, int64_t size) {
        LOG_INFO("Encoder progress - Frames: " + std::to_string(frames) +
                 ", Size: " + std::to_string(size));
        if (progressCallback_) {
            progressCallback_(frames, size);
        }
    });

    encoder_->setErrorCallback([this](const std::string& errorMessage) {
        lastError_ = errorMessage;
        LOG_ERROR("Encoder error: " + errorMessage);
        if (errorCallback_) {
            errorCallback_(errorMessage);
        }
    });

    grabberThread_->start();

    encoder_->start();

    return true;
}

void ScreenRecorder::stopRecording() {
    if (!isRecording_.load()) {
        return;  // 不在录制中
    }

    LOG_INFO("[ScreenRecorder] Stopping Recording");

    // 1. 先停止依赖外部资源的异步线程，防止竞态条件（如主循环还在写入 RingBuffer 或发送 ZMQ 消息）
    stopPublishing();
    stopKeyFrameMetaDataReceiving();
    if (grabberThread_) {
        grabberThread_->stop();
        grabberThread_.reset();
    }

    if (audioGrabber_) {
        audioGrabber_->stop();
        audioGrabber_.reset();
    }

    if (encoder_) {
        encoder_->stop();
        encoder_.reset();
    }

    if (audioEncoder_) {
        audioEncoder_->stop();
        audioEncoder_.reset();
    }

    if (ffmpegWrapper_) {
        ffmpegWrapper_->finalize();
        ffmpegWrapper_.reset();
    }

    frameQueue_.reset();
    audioQueue_.reset();
    videoRingBuffer_.reset();
    grabber_.reset();

    isRecording_.store(false);
    LOG_INFO("[ScreenRecorder] Recording Stopped");
}

void ScreenRecorder::pauseRecording() {
    if (grabberThread_) {
        grabberThread_->pause();
    }
}

void ScreenRecorder::resumeRecording() {
    if (grabberThread_) {
        grabberThread_->resume();
    }
}

int64_t ScreenRecorder::getFrameCount() const {
    if (grabberThread_) {
        return grabberThread_->getCapturedFrameCount();
    }
    return 0;
}

int64_t ScreenRecorder::getEncodedCount() const {
    if (encoder_) {
        return encoder_->getEncodedFrameCount();
    }
    return 0;
}

int64_t ScreenRecorder::getDroppedCount() const {
    if (grabberThread_) {
        return grabberThread_->getDroppedFrameCount();
    }
    return 0;
}

int64_t ScreenRecorder::getOutputFileSize() const {
    if (ffmpegWrapper_) {
        return ffmpegWrapper_->getOutputFileSize();
    }
    return 0;
}

double ScreenRecorder::getCurrentFps() const {
    if (grabberThread_) {
        return grabberThread_->getCurrentFps();
    }
    return 0.0;
}

bool ScreenRecorder::isRecording() const {
    return isRecording_.load();
}

void ScreenRecorder::setProgressCallback(ProgressCallback callback) {
    progressCallback_ = callback;
}

void ScreenRecorder::setErrorCallback(ErrorCallback callback) {
    errorCallback_ = callback;
}
void ScreenRecorder::setRecorderMode(RecorderMode mode) {
    mode_ = mode;
}

RecorderMode ScreenRecorder::getRecorderMode() const {
    return mode_;
}

//================发布线程================//
bool ScreenRecorder::startPublishing() {
    if (publishingRunning_)
        return false;
    publishingRunning_ = true;
    publishingThread_ = std::thread(&ScreenRecorder::publishingLoop, this);
    return true;
}

void ScreenRecorder::stopPublishing() {
    publishingRunning_ = false;
    publishCondVar_.notify_all();
    if (publishingThread_.joinable()) {
        publishingThread_.join();
    }
}

void ScreenRecorder::pushToPublishQueue(const FrameData& data) {
    {
        std::lock_guard<std::mutex> lock(publishMutex_);
        if (publishQueue_.size() >= 30) {
            publishQueue_.pop();
        }
        publishQueue_.push(data);  // 浅拷贝 FrameData，包含共享指针和Mat引用
    }
    publishCondVar_.notify_one();
}

void ScreenRecorder::publishingLoop() {
    framePublisher_ = std::make_unique<MQInfra::FramePublisher>();

    if (!framePublisher_->initialize("tcp://*:5555")) {
        LOG_ERROR("Failed to initialize FramePublisher.");
        return;
    }

    LOG_INFO("Publish thread Started");

    while (publishingRunning_) {
        FrameData frame;
        {
            std::unique_lock<std::mutex> lock(publishMutex_);

            if (!publishCondVar_.wait_for(lock, std::chrono::milliseconds(100), [this] {
                    return !publishQueue_.empty() || !publishingRunning_;
                })) {
                continue;  // 超时，继续等待
            }

            if (!publishingRunning_)
                break;

            frame = std::move(publishQueue_.front());
            publishQueue_.pop();
        }

        // 同时推送到环形缓冲区，供关键帧编码使用
        if (videoRingBuffer_) {
            videoRingBuffer_->push(frame.frame_ID, frame.frame, frame.timestamp_ms);
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
        if (!framePublisher_->publishRaw(msgheader, frame.frame.data, dataSize, crc32)) {
            LOG_ERROR("Failed to publish frame ID: " + std::to_string(frame.frame_ID));
        }
    }
    framePublisher_->shutdown();
    LOG_INFO("Publish Thread Stopped");
}

//================接受线程================//
bool ScreenRecorder::startKeyFrameMetaDataReceiving(const std::string& keyFramePath) {
    if (receivingRunning_) {
        return false;  // 已经在接收中
    }

    keyFrameOutputPath_ = keyFramePath;

    // 初始化关键帧编码器
    if (grabber_) {
        EncoderConfig config = encoderConfigFromGrabber(grabber_.get());
        config.video.outputFilePath = keyFrameOutputPath_;
        // 关键帧模式下，通常我们希望高质量，可以根据需要调整 FPS 或码率
        // 这里暂时复用主配置，但路径不同

        keyFrameFFmpegWrapper_ = std::make_shared<FFmpegWrapper>();
        if (!keyFrameFFmpegWrapper_->initialize(config)) {
            LOG_ERROR("Failed to initialize KeyFrame FFmpegWrapper: " +
                      keyFrameFFmpegWrapper_->getLastError());
            return false;
        }

        if (config.audio.enabled) {
            keyFrameAudioQueue_ = std::make_shared<ThreadSafetyQueue<AudioData>>(100);
            keyFrameAudioEncoder_ =
                std::make_unique<AudioEncoder>(keyFrameAudioQueue_, keyFrameFFmpegWrapper_);
            keyFrameAudioEncoder_->start();
        }
    }

    keyFrameMetaDataSubscriber_ = std::make_unique<MQInfra::KeyFrameMetaDataSubscriber>();

    if (!keyFrameMetaDataSubscriber_->initialize("tcp://localhost:5556")) {
        LOG_ERROR("Failed to initialize KeyFrameMetaDataSubscriber.");
        keyFrameMetaDataSubscriber_.reset();
        return false;
    }

    receivingRunning_ = true;
    keyFrameReceiveThread_ = std::thread(&ScreenRecorder::keyFrameMetaDataReceiveLoop, this);

    return true;
}

bool ScreenRecorder::stopKeyFrameMetaDataReceiving() {
    if (!keyFrameMetaDataSubscriber_) {
        return false;  // 不在接收中
    }

    receivingRunning_ = false;
    keyFrameMetaDataCondVar_.notify_all();
    if (keyFrameReceiveThread_.joinable()) {
        keyFrameReceiveThread_.join();
    }

    if (keyFrameAudioEncoder_) {
        keyFrameAudioEncoder_->stop();
        keyFrameAudioEncoder_.reset();
    }

    if (keyFrameFFmpegWrapper_) {
        keyFrameFFmpegWrapper_->finalize();
        keyFrameFFmpegWrapper_.reset();
    }

    if (keyFrameAudioQueue_) {
        keyFrameAudioQueue_->stop();
        keyFrameAudioQueue_.reset();
    }

    keyFrameMetaDataSubscriber_->shutdown();
    keyFrameMetaDataSubscriber_.reset();

    return true;
}

void ScreenRecorder::keyFrameMetaDataReceiveLoop() {
    LOG_INFO("Key Frame MetaData Receive Thread Started");

    while (receivingRunning_) {
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
        if (keyFrameFFmpegWrapper_ && videoRingBuffer_) {
            uint64_t timestamp = 0;
            auto frameMat = videoRingBuffer_->get(header.frameID, timestamp);
            if (frameMat) {
                FrameData fd;
                fd.frame = *frameMat;
                fd.frame_ID = header.frameID;
                fd.timestamp_ms = timestamp;

                if (!keyFrameFFmpegWrapper_->encoderFrame(fd)) {
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