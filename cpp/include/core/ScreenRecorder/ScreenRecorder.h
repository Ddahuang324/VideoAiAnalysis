#pragma once

#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <memory>
#include <mutex>
#include <opencv2/core/mat.hpp>
#include <queue>
#include <string>
#include <thread>

#include "AudioData.h"
#include "AudioEncoder.h"
#include "AudioGrabber.h"
#include "FFmpegWrapper.h"
#include "FrameEncoder.h"
#include "FrameGrabberThread.h"
#include "FramePublisher.h"
#include "KeyFrameMetaDataSubscriber.h"
#include "Protocol.h"
#include "RingFrameBuffer.h"
#include "ThreadSafetyQueue.h"
#include "VideoGrabber.h"

enum class RecorderMode {
    VIDEO,
    SNAPSHOT
};

class ScreenRecorder {
public:
    ScreenRecorder();
    ~ScreenRecorder();

    bool startRecording(std::string& path);
    void stopRecording();
    void pauseRecording();
    void resumeRecording();

    int64_t getFrameCount() const;
    int64_t getEncodedCount() const;
    int64_t getDroppedCount() const;
    int64_t getOutputFileSize() const;
    double getCurrentFps() const;

    bool isRecording() const;

    void setRecorderMode(RecorderMode mode);
    RecorderMode getRecorderMode() const;

    using ProgressCallback = FrameEncoder::ProgressCallback;
    using ErrorCallback = FrameEncoder::ErrorCallback;
    using FrameCallback = FrameGrabberThread::FrameCallback;

    void setProgressCallback(ProgressCallback callback);
    void setErrorCallback(ErrorCallback callback);
    void setFrameCallback(FrameCallback callback);

    std::string getLastError() const { return lastError_; }

    bool startPublishing();
    void stopPublishing();
    void pushToPublishQueue(const FrameData& frame);

    bool startKeyFrameMetaDataReceiving(const std::string& keyFramePath);
    bool stopKeyFrameMetaDataReceiving();

private:
    void publishingLoop();
    void keyFrameMetaDataReceiveLoop();

    // Core recording components
    std::shared_ptr<VideoGrabber> grabber_;
    std::shared_ptr<AudioGrabber> audioGrabber_;
    std::shared_ptr<FFmpegWrapper> ffmpegWrapper_;
    std::unique_ptr<FrameEncoder> encoder_;
    std::unique_ptr<AudioEncoder> audioEncoder_;
    std::shared_ptr<ThreadSafetyQueue<FrameData>> frameQueue_;
    std::shared_ptr<ThreadSafetyQueue<AudioData>> audioQueue_;
    std::shared_ptr<FrameGrabberThread> grabberThread_;

    // Recording state
    std::atomic<bool> isRecording_{false};
    RecorderMode mode_{RecorderMode::VIDEO};
    std::string lastError_;
    ProgressCallback progressCallback_;
    ErrorCallback errorCallback_;
    FrameCallback frameCallback_;

    // Publishing thread
    std::thread publishingThread_;
    std::queue<FrameData> publishQueue_;
    std::mutex publishMutex_;
    std::condition_variable publishCondVar_;
    std::unique_ptr<MQInfra::FramePublisher> framePublisher_;
    std::atomic<bool> publishingRunning_{false};

    // Key frame receiving thread
    std::thread keyFrameReceiveThread_;
    std::queue<Protocol::KeyFrameMetaDataHeader> keyFrameMetaDataQueue_;
    std::mutex keyFrameMetaDataMutex_;
    std::condition_variable keyFrameMetaDataCondVar_;
    std::unique_ptr<MQInfra::KeyFrameMetaDataSubscriber> keyFrameMetaDataSubscriber_;
    std::atomic<bool> receivingRunning_{false};

    // Key frame encoding components
    std::shared_ptr<FFmpegWrapper> keyFrameFFmpegWrapper_;
    std::shared_ptr<ThreadSafetyQueue<AudioData>> keyFrameAudioQueue_;
    std::unique_ptr<AudioEncoder> keyFrameAudioEncoder_;
    std::unique_ptr<RingFrameBuffer> videoRingBuffer_;
    std::string keyFrameOutputPath_;
};