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
    VIDEO,    // 视频模式 (普通帧率, 如 30fps)
    SNAPSHOT  // 快照模式 (低帧率, 如 1fps)
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

    bool is_Recording() const;

    void setRecorderMode(RecorderMode mode);
    RecorderMode getRecorderMode() const;

    using ProgressCallback = FrameEncoder::ProgressCallback;
    using ErrorCallback = FrameEncoder::ErrorCallback;
    using FrameCallback = FrameGrabberThread::FrameCallback;

    void setProgressCallback(ProgressCallback callback);
    void setErrorCallback(ErrorCallback callback);
    void setFrameCallback(FrameCallback callback);

    std::string getLastError() const { return m_lastError; }

    //================发布线程================//
    bool startPublishing();
    void stopPublishing();
    void pushToPublishQueue(const FrameData& frame);

    //================接受线程================//
    bool startKeyFrameMetaDataReceiving(const std::string& keyFramePath);
    bool stopKeyFrameMetaDataReceiving();

private:
    std::shared_ptr<VideoGrabber> m_grabber_;  // 改为 shared_ptr 以便与 FrameGrabberThread 共享
    std::shared_ptr<AudioGrabber> m_audioGrabber_;

    std::shared_ptr<FFmpegWrapper> m_ffmpegWrapper_;
    std::unique_ptr<FrameEncoder> m_encoder_;
    std::unique_ptr<AudioEncoder> m_audioEncoder_;

    std::shared_ptr<ThreadSafetyQueue<FrameData>> m_frameQueue_;
    std::shared_ptr<ThreadSafetyQueue<AudioData>> m_audioQueue_;

    std::shared_ptr<FrameGrabberThread> grabber_thread_;
    std::atomic<bool> m_isRecording{false};
    RecorderMode m_mode{RecorderMode::VIDEO};
    std::string m_lastError;
    ProgressCallback m_progressCallback;
    ErrorCallback m_errorCallback;
    FrameCallback m_frameCallback;

    //================发布线程================//

    std::thread m_publishingThread;
    std::queue<FrameData> m_publishqueue;
    std::mutex m_publishMutex;
    std::condition_variable m_publishCondVar;
    std::unique_ptr<MQInfra::FramePublisher> m_framePublisher;
    std::atomic<bool> publishrunning_{false};

    void publishingLoop();

    //================接受线程================//

    std::thread keyFrameReceiveThread_;
    std::queue<Protocol::KeyFrameMetaDataHeader> keyFrameMetaDataQueue_;
    std::mutex keyFrameMetaDataMutex_;
    std::condition_variable keyFrameMetaDataCondVar_;
    std::unique_ptr<MQInfra::KeyFrameMetaDataSubscriber> keyFrameMetaDataSubscriber_;
    std::atomic<bool> receiverunning_{false};

    void keyFrameMetaDataReceiveLoop();

    //==================关键帧编码===================//
    std::shared_ptr<FFmpegWrapper> m_keyFrameFFmpegWrapper_;
    std::shared_ptr<ThreadSafetyQueue<AudioData>> m_keyFrameAudioQueue_;
    std::unique_ptr<AudioEncoder> m_keyFrameAudioEncoder_;
    std::unique_ptr<RingFrameBuffer> m_videoRingBuffer_;
    std::string m_keyFrameOutputPath_;

    //==================编码线程===================//
};