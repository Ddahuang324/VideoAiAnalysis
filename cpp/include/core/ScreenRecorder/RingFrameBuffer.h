#pragma once

#include <opencv2/core/hal/interface.h>

#include <cstddef>
#include <cstdint>
#include <mutex>
#include <opencv2/core/mat.hpp>
#include <optional>
#include <vector>

class RingFrameBuffer {
public:
    struct Stats {
        size_t total_written_frames;
        size_t total_read_frames;
        size_t total_overwrite_frames;
    };
    explicit RingFrameBuffer(size_t capacity = 30);

    void push(uint32_t frameID, const cv::Mat& frame, uint64_t timestamp_ms);

    std::optional<cv::Mat> get(uint32_t frameID, uint64_t& out_timestamp_ms);

private:
    struct Slot {
        uint32_t frameID;
        uint64_t timestamp_ms;
        cv::Mat frame;
        bool valid;
    };

    size_t capacity_;
    std::vector<Slot> buffer_;
    mutable std::mutex mutex_;
    Stats stats_{0, 0, 0};
};