#include "FrameGrabberThread.h"

#include <chrono>
#include <cstdint>
#include <exception>
#include <memory>
#include <mutex>
#include <thread>

#include "Log.h"
#include "ThreadSafetyQueue.h"
#include "VideoGrabber.h"

namespace {

constexpr int DEFAULT_FRAME_TIMEOUT_MS = 100;
constexpr int PROGRESS_CALLBACK_INTERVAL = 1;  // Callback every target_fps frames

}  // namespace

FrameGrabberThread::FrameGrabberThread(std::shared_ptr<VideoGrabber> grabber,
                                       ThreadSafetyQueue<FrameData>& queue,
                                       int target_fps)
    : grabber_(std::move(grabber)),
      frame_queue_(queue),
      m_thread(nullptr),
      target_fps_(target_fps) {}

FrameGrabberThread::~FrameGrabberThread() {
    if (running_) {
        LOG_WARN("FrameGrabberThread destroyed while still running, calling stop()");
        stop();
    }
}

void FrameGrabberThread::start() {
    if (running_) {
        LOG_WARN("Thread is already running");
        return;
    }

    if (!grabber_->start()) {
        LOG_ERROR("Failed to start grabber");
        notifyError("Failed to start grabber");
        return;
    }

    resetStatistics();

    running_ = true;
    paused_ = false;

    m_thread = std::make_unique<std::thread>(&FrameGrabberThread::captureLoop, this);
    LOG_INFO("FrameGrabberThread started");
}

void FrameGrabberThread::stop() {
    if (!running_) {
        LOG_WARN("Thread is not running");
        return;
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
    grabber_->pause();
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
    pause_cv_.notify_one();
    grabber_->resume();
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

void FrameGrabberThread::resetStatistics() {
    captured_frame_count_ = 0;
    dropped_frame_count_ = 0;
    current_fps_ = 0.0;
    std::lock_guard<std::mutex> lock(fps_mutex_);
    fps_samples.clear();
}

void FrameGrabberThread::captureLoop() {
    start_time_ = std::chrono::steady_clock::now();
    last_frame_time_ = start_time_;

    LOG_INFO("Capture loop started");

    constexpr auto QUEUE_PUSH_TIMEOUT = std::chrono::milliseconds(100);

    while (running_) {
        if (paused_) {
            std::unique_lock<std::mutex> lock(pause_mutex_);
            pause_cv_.wait(lock, [this] { return !paused_ || !running_; });
            if (!running_) {
                break;
            }
            continue;
        }

        capture_start_ = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - start_time_);

        FrameData frame = grabber_->CaptureFrame(DEFAULT_FRAME_TIMEOUT_MS);

        if (!frame.data) {
            LOG_WARN("Failed to capture frame");
            notifyError("Failed to capture frame");
            continue;
        }

        frame.timestamp_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                                 std::chrono::steady_clock::now() - start_time_)
                                 .count();
        frame.frame_ID = captured_frame_count_.load() + 1;

        if (!frame_queue_.push(frame, QUEUE_PUSH_TIMEOUT)) {
            dropped_frame_count_++;
            LOG_WARN("Frame dropped: queue full");
            notifyDropped(dropped_frame_count_);
        } else {
            captured_frame_count_++;
            updateFps();

            if (frame_callback_) {
                frame_callback_(frame);
            }

            if (captured_frame_count_.load() % target_fps_ == 0) {
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

void FrameGrabberThread::updateFps() {
    auto now = std::chrono::steady_clock::now();

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