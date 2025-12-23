#pragma once

#include <atomic>
#include <memory>
#include <thread>

#include "AudioData.h"
#include "FFmpegWrapper.h"
#include "ThreadSafetyQueue.h"

/**
 * @brief 音频编码线程类
 * 负责从队列中提取 AudioData 并调用 FFmpegWrapper 进行编码
 */
class AudioEncoder {
public:
    AudioEncoder(std::shared_ptr<ThreadSafetyQueue<AudioData>> queue,
                 std::shared_ptr<FFmpegWrapper> encoder);

    ~AudioEncoder();

    // 禁止拷贝构造和赋值
    AudioEncoder(const AudioEncoder&) = delete;
    AudioEncoder& operator=(const AudioEncoder&) = delete;

    // 公共接口
    void start();
    void stop();

    bool isRunning() const { return isRunning_.load(); }

private:
    void encodeLoop();

    std::shared_ptr<ThreadSafetyQueue<AudioData>> queue_;
    std::shared_ptr<FFmpegWrapper> encoder_;
    std::unique_ptr<std::thread> thread_;

    std::atomic<bool> isRunning_{false};
};
