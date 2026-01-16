#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <opencv2/core.hpp>
#include <opencv2/core/mat.hpp>

enum class PixelFormat {
    UNKNOWN,
    BGRA,
    RGBA,
    RGB24,
};

struct FrameData {
    uint8_t* data = nullptr;
    int width = 0;
    int height = 0;
    PixelFormat format = PixelFormat::UNKNOWN;
    int64_t timestamp_ms = 0;
    uint32_t frame_ID = 0;
    cv::Mat frame;
    std::shared_ptr<uint8_t> data_holder;
};

inline FrameData CreateFrameData(int width, int height) {
    FrameData frame;
    frame.width = width;
    frame.height = height;

    const size_t size = width * height * 4;
    frame.data_holder = std::shared_ptr<uint8_t>(new uint8_t[size], std::default_delete<uint8_t[]>());
    frame.data = frame.data_holder.get();

    return frame;
}

class VideoGrabber {
public:
    virtual ~VideoGrabber() = default;

    // 开始采集
    virtual bool start() = 0;

    // 停止采集
    virtual void stop() = 0;

    // 暂停采集
    virtual void pause() = 0;

    // 恢复采集
    virtual void resume() = 0;

    virtual int getWidth() const = 0;
    virtual int getHeight() const = 0;
    virtual int getFps() const = 0;
    virtual PixelFormat getPixelFormat() const = 0;

    virtual bool isRunning() const = 0;
    virtual bool isPaused() const = 0;

    virtual FrameData CaptureFrame(int timeout_ms = 100) = 0;
};
