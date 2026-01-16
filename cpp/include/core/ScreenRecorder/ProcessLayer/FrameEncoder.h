#pragma once

#include <atomic>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <thread>
#include <utility>

#include "FFmpegWrapper.h"
#include "ThreadSafetyQueue.h"
#include "VideoGrabber.h"


class FrameEncoder {
public:
    FrameEncoder(std::shared_ptr<ThreadSafetyQueue<FrameData>> queue,
                 std::shared_ptr<FFmpegWrapper> encoder, const EncoderConfig& config);

    ~FrameEncoder();

    // 禁止拷贝构造和赋值
    FrameEncoder(const FrameEncoder&) = delete;
    FrameEncoder& operator=(const FrameEncoder&) = delete;

    // 回调类型定义
    using ProgressCallback = std::function<void(int64_t frames, int64_t Size)>;
    using FinishedCallback = std::function<void(int64_t totalFrames, const std::string& path)>;
    using ErrorCallback = std::function<void(const std::string& errorMessage)>;

    // 公共接口
    void start();

    void stop();

    bool isRunning() const { return isRunning_.load(); }

    // 状态查询
    int64_t getEncodedFrameCount() const { return encodedFrameCount_.load(); }
    int64_t getOutputFileSize() const;

    // 回调设置方法
    void setProgressCallback(ProgressCallback callback) { progressCallback_ = std::move(callback); }
    void setFinishedCallback(FinishedCallback callback) { finishedCallback_ = std::move(callback); }
    void setErrorCallback(ErrorCallback callback) { errorCallback_ = std::move(callback); }

private:
    // 编码线程主循环
    void encodeLoop();
    bool processFrame(const FrameData& frameData);

    // 回调通知
    void notifyProgress();
    void notifyFinished();
    void notifyError(const std::string& errorMessage);

    // 成员变量
    std::shared_ptr<ThreadSafetyQueue<FrameData>> queue_;
    std::shared_ptr<FFmpegWrapper> encoder_;
    std::unique_ptr<std::thread> thread_;
    EncoderConfig config_;

    // 状态标志
    std::atomic<bool> isRunning_{false};
    std::atomic<int64_t> encodedFrameCount_{0};

    // 回调
    ProgressCallback progressCallback_;
    FinishedCallback finishedCallback_;
    ErrorCallback errorCallback_;
};
