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
    uint8_t* data;
    int width;
    int height;
    PixelFormat format;
    int64_t timestamp_ms;
    uint32_t frame_ID;
    cv::Mat frame;
    std::shared_ptr<uint8_t> data_holder;

    FrameData()
        : data(nullptr), width(0), height(0), format(PixelFormat::UNKNOWN), timestamp_ms(0) {}
};

inline FrameData CaptureFrame(int width, int height) {
    FrameData frame;
    frame.width = width;
    frame.height = height;

    size_t size = width * height * 4;
    // 使用 shared_ptr 构造函数接管原生指针，并指定数组删除器
    frame.data_holder = std::shared_ptr<uint8_t>(new uint8_t[size], [](uint8_t* p) { delete[] p; });
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
