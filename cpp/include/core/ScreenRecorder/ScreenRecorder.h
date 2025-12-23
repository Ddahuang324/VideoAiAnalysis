#pragma once

#include <atomic>
#include <cstdint>
#include <memory>
#include <string>

#include "FrameEncoder.h"
#include "FrameGrabberThread.h"
#include "IScreenGrabber.h"
#include "ThreadSafetyQueue.h"

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

private:
    std::shared_ptr<IScreenGrabber> m_grabber_;  // 改为 shared_ptr 以便与 FrameGrabberThread 共享
    std::unique_ptr<FrameEncoder> m_encoder_;
    std::shared_ptr<ThreadSafetyQueue<FrameData>> m_frameQueue_;
    std::shared_ptr<FrameGrabberThread> grabber_thread_;
    std::atomic<bool> m_isRecording{false};
    RecorderMode m_mode{RecorderMode::VIDEO};
    std::string LastError;
    ProgressCallback m_progressCallback;
    ErrorCallback m_errorCallback;
    FrameCallback m_frameCallback;
};