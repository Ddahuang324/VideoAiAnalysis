#include "FrameGrabberThread.h"

#include <chrono>
#include <cstdint>
#include <exception>
#include <memory>
#include <mutex>
#include <string>
#include <thread>

#include "IScreenGrabber.h"
#include "Log.h"
#include "ThreadSafetyQueue.h"

FrameGrabberThread::FrameGrabberThread(std::shared_ptr<IScreenGrabber> grabber,
                                       ThreadSafetyQueue<FrameData>& queue, int target_fps)
    : grabber_(grabber), frame_queue_(queue), m_thread(nullptr), target_fps_(target_fps) {}

FrameGrabberThread::~FrameGrabberThread() {
    if (running_) {
        LOG_WARN("FrameGrabberThread destroyed while still running, calling stop()");
        stop();
    }
}

void FrameGrabberThread::start() {
    if (running_) {
        LOG_WARN("Thread is already Running");
        return;  // 已经在运行
    }

    if (!grabber_->start()) {
        LOG_ERROR("Failed to start grabber");
        notifyError("Failed to start grabber");
        return;
    }

    // 重置统计数据（支持stop后重新start）
    captured_frame_count_ = 0;
    dropped_frame_count_ = 0;
    current_fps_ = 0.0;
    {
        std::lock_guard<std::mutex> lock(fps_mutex_);
        fps_samples.clear();
    }

    running_ = true;
    paused_ = false;

    m_thread = std::make_unique<std::thread>(&FrameGrabberThread::captureLoop, this);
    LOG_INFO("FrameGrabberThread started");
}

void FrameGrabberThread::stop() {
    if (!running_) {
        LOG_WARN("Thread is not running");
        return;  // 已经停止
    }

    running_ = false;
    if (m_thread && m_thread->joinable()) {
        m_thread->join();
    }

    grabber_->stop();
    LOG_INFO("FrameGrabberThread stopped");
}

void FrameGrabberThread::pause() {
    if (!running_) {
        LOG_WARN("Thread is not running, cannot pause");
        return;
    }
    paused_ = true;
    grabber_->pause();  // 同时暂停底层采集器
    LOG_INFO("FrameGrabberThread paused");
}

void FrameGrabberThread::resume() {
    if (!running_) {
        LOG_WARN("Thread is not running, cannot resume");
        return;
    }
    {
        std::lock_guard<std::mutex> lock(pause_mutex_);
        paused_ = false;
    }
    pause_cv_.notify_one();  // 唤醒等待的线程
    grabber_->resume();      // 同时恢复底层采集器
    LOG_INFO("FrameGrabberThread resumed");
}

int64_t FrameGrabberThread::getCapturedFrameCount() const {
    return captured_frame_count_.load();
}

int64_t FrameGrabberThread::getDroppedFrameCount() const {
    return dropped_frame_count_.load();
}

double FrameGrabberThread::getCurrentFps() const {
    return current_fps_.load();
}

bool FrameGrabberThread::isRunning() const {
    return running_.load();
}

bool FrameGrabberThread::isPaused() const {
    return paused_.load();
}

void FrameGrabberThread::captureLoop() {
    start_time_ = std::chrono::steady_clock::now();
    last_frame_time_ = start_time_;

    LOG_INFO("Capture loop started");

    while (running_) {
        // 优化的暂停机制：使用条件变量而非轮询
        if (paused_) {
            std::unique_lock<std::mutex> lock(pause_mutex_);
            pause_cv_.wait(lock, [this] { return !paused_ || !running_; });
            if (!running_)
                break;  // 在暂停期间被停止
            continue;
        }

        capture_start_ = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - start_time_);

        FrameData frame;
        frame = grabber_->CaptureFrame(100);

        if (!frame.data) {
            LOG_WARN("Failed to capture frame");
            notifyError("Failed to capture frame");  // 通知Python层
            continue;
        }

        frame.timestamp_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                                 std::chrono::steady_clock::now() - start_time_)
                                 .count();

        if (!frame_queue_.push(frame, std::chrono::milliseconds(100))) {
            dropped_frame_count_++;
            LOG_WARN("Frame dropped: queue full");
            notifyDropped(dropped_frame_count_);

        } else {
            captured_frame_count_++;
            UpdateFps();

            if (captured_frame_count_ % target_fps_ == 0) {  // 减少回调频率
                notifyProgress();
            }
        }

        waitForNextFrame();
    }

    LOG_INFO("Capture loop ended");
}

void FrameGrabberThread::waitForNextFrame() {
    auto now = std::chrono::steady_clock::now();
    auto elapsed = now - last_frame_time_;
    auto frame_duration = std::chrono::milliseconds(1000 / target_fps_);

    if (elapsed < frame_duration) {
        auto sleep_duration = frame_duration - elapsed;
        std::this_thread::sleep_for(sleep_duration);
    }

    last_frame_time_ = std::chrono::steady_clock::now();
}

void FrameGrabberThread::UpdateFps() {
    auto now = std::chrono::steady_clock::now();

    // 线程安全：保护fps_samples的访问
    std::lock_guard<std::mutex> lock(fps_mutex_);

    fps_samples.push_back(now);

    if (fps_samples.size() > FPS_SAMPLE_SIZE) {
        fps_samples.pop_front();
    }

    if (fps_samples.size() >= 2) {
        auto duration = fps_samples.back() - fps_samples.front();
        auto seconds = std::chrono::duration<double>(duration).count();

        current_fps_ = (fps_samples.size() - 1) / seconds;
    }
}

void FrameGrabberThread::notifyProgress() {
    if (!progress_callback_) {
        return;
    }

    try {
        progress_callback_(captured_frame_count_.load(), frame_queue_.size(), current_fps_.load());
    } catch (const std::exception& e) {
        LOG_ERROR(std::string("Error in progress callback: ") + e.what());
    }
}

void FrameGrabberThread::notifyError(const std::string& error_msg) {
    if (!error_callback_) {
        return;
    }

    try {
        error_callback_(error_msg);
    } catch (const std::exception& e) {
        LOG_ERROR(std::string("Error in error callback: ") + e.what());
    }
}

void FrameGrabberThread::notifyDropped(int64_t dropped_count) {
    if (!dropped_callback_) {
        return;
    }

    try {
        dropped_callback_(dropped_count);
    } catch (const std::exception& e) {
        LOG_ERROR(std::string("Error in dropped callback: ") + e.what());
    }
}
