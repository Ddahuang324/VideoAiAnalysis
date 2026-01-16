#include "core/ScreenRecorder/RingFrameBuffer.h"

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <mutex>
#include <opencv2/core/mat.hpp>
#include <optional>
#include <string>

#include "Log.h"

RingFrameBuffer::RingFrameBuffer(size_t capacity) : capacity_(capacity), buffer_(capacity) {
    // Slot::valid is default-initialized to false by vector constructor
}

void RingFrameBuffer::push(uint32_t frameID, const cv::Mat& frame, uint64_t timestamp_ms) {
    std::lock_guard<std::mutex> lock(mutex_);

    size_t index = frameID % capacity_;

    if (buffer_[index].valid && buffer_[index].frameID != frameID) {
        stats_.total_overwrite_frames++;
    }

    buffer_[index].frameID = frameID;
    buffer_[index].timestamp_ms = timestamp_ms;
    buffer_[index].frame = frame.clone();  // 深拷贝
    buffer_[index].valid = true;

    stats_.total_written_frames++;
}

std::optional<cv::Mat> RingFrameBuffer::get(uint32_t frameID, uint64_t& out_timestamp_ms) {
    std::lock_guard<std::mutex> lock(mutex_);

    size_t index = frameID % capacity_;

    if (buffer_[index].valid && buffer_[index].frameID == frameID) {
        stats_.total_read_frames++;
        out_timestamp_ms = buffer_[index].timestamp_ms;
        return buffer_[index].frame.clone();  // 返回深拷贝
    } else {
        LOG_WARN("Frame ID " + std::to_string(frameID) + " not found in buffer.");
        return std::nullopt;
    }
}
