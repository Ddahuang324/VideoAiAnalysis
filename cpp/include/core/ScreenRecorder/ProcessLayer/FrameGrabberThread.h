#pragma once

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <deque>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <thread>

#include "IScreenGrabber.h"
#include "ThreadSafetyQueue.h"

class FrameGrabberThread {
public:
    FrameGrabberThread(std::shared_ptr<IScreenGrabber> grabber, ThreadSafetyQueue<FrameData>& queue,
                       int target_fps = 60);

    ~FrameGrabberThread();

    // 禁止拷贝
    FrameGrabberThread(const FrameGrabberThread&) = delete;
    FrameGrabberThread& operator=(const FrameGrabberThread&) = delete;

    void start();   // 启动采集线程
    void stop();    // 停止采集线程
    void pause();   // 暂停采集
    void resume();  // 恢复采集

    int64_t getCapturedFrameCount() const;  // 获取已采集的帧数
    int64_t getDroppedFrameCount() const;   // 获取丢弃的帧数
    double getCurrentFps() const;           // 获取当前采集的帧率
    bool isRunning() const;                 // 采集线程是否在运行
    bool isPaused() const;                  // 采集是否已暂停

    using ProgressCallback = std::function<void(int64_t, int, double)>;  // Pybind回调
    using ErrorCallback = std::function<void(const std::string&)>;       // Pybind回调
    using DroppedCallback = std::function<void(int64_t)>;                // Pybind回调

    void setProgressCallback(ProgressCallback callback) {
        progress_callback_ = callback;
    }  // 设置进度回调,python绑定使用
    void setErrorCallback(ErrorCallback callback) {
        error_callback_ = callback;
    };  // 设置错误回调,python绑定使用
    void setDroppedCallback(DroppedCallback callback) {
        dropped_callback_ = callback;
    };  // 设置丢帧回调,python绑定使用

private:
    void captureLoop();       // 采集线程主循环
    void UpdateFps();         // 更新当前帧率
    void waitForNextFrame();  // 等待下一帧采集时间

    void notifyProgress();                           // 通知进度回调
    void notifyError(const std::string& error_msg);  // 通知错误回调
    void notifyDropped(int64_t dropped_count);       // 通知丢帧回调

    // 核心资源
    std::shared_ptr<IScreenGrabber> grabber_;    // 屏幕采集器
    ThreadSafetyQueue<FrameData>& frame_queue_;  // 帧数据队列
    std::unique_ptr<std::thread> m_thread;       // 采集线程

    int target_fps_;  // 目标采集帧率

    std::atomic<bool> running_{false};  // 采集线程运行状态
    std::atomic<bool> paused_{false};   // 采集暂停状态

    // 线程同步（暂停机制优化）
    std::mutex pause_mutex_;
    std::condition_variable pause_cv_;

    // 统计数据
    std::atomic<int64_t> captured_frame_count_{0};  // 已采集帧数
    std::atomic<int64_t> dropped_frame_count_{0};   // 丢弃
    std::atomic<double> current_fps_{0.0};          // 当前采集帧率

    // 时间戳
    std::chrono::steady_clock::time_point start_time_;
    std::chrono::steady_clock::time_point last_frame_time_;
    std::chrono::milliseconds frame_interval_;
    std::chrono::milliseconds capture_start_;

    // FPS 计算
    static constexpr int FPS_SAMPLE_SIZE = 60;                      // 采样数量
    std::deque<std::chrono::steady_clock::time_point> fps_samples;  // 采样时间点
    std::mutex fps_mutex_;                                          // fps_samples的线程安全保护

    // 回调函数
    ProgressCallback progress_callback_;
    ErrorCallback error_callback_;
    DroppedCallback dropped_callback_;
};