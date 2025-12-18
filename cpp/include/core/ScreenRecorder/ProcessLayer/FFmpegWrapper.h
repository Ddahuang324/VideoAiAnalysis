#pragma once

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>
#include <libavutil/opt.h>
#include <libswscale/swscale.h>

#include "libavcodec/packet.h"
#include "libavformat/avio.h"
#include "libavutil/frame.h"
}

#include <cstdint>
#include <memory>
#include <string>

#include "IScreenGrabber.h"

struct EncoderConfig;
struct AVFormatContextDeleter {
    void operator()(AVFormatContext* ptr) noexcept {
        if (ptr) {
            if (ptr->pb) {
                avio_closep(&ptr->pb);
            }
            avformat_free_context(ptr);
        }
    }
};

struct AVCodecContextDeleter {
    void operator()(AVCodecContext* ptr) noexcept {
        if (ptr) {
            avcodec_free_context(&ptr);
        }
    }
};

struct AVFrameDeleter {
    void operator()(AVFrame* ptr) noexcept {
        if (ptr) {
            av_frame_free(&ptr);
        }
    }
};

struct AVPacketDeleter {
    void operator()(AVPacket* ptr) noexcept {
        if (ptr) {
            av_packet_free(&ptr);
        }
    }
};

struct SwsContextDeleter {
    void operator()(SwsContext* ptr) noexcept {
        if (ptr) {
            sws_freeContext(ptr);
        }
    }
};

using AVFormatContextPtr = std::unique_ptr<AVFormatContext, AVFormatContextDeleter>;
using AVCodecContextPtr = std::unique_ptr<AVCodecContext, AVCodecContextDeleter>;
using AVFramePtr = std::unique_ptr<AVFrame, AVFrameDeleter>;
using AVPacketPtr = std::unique_ptr<AVPacket, AVPacketDeleter>;
using SwsContextPtr = std::unique_ptr<SwsContext, SwsContextDeleter>;

class FFmpegWrapper {
public:
    FFmpegWrapper();
    ~FFmpegWrapper();

    // 禁止拷贝构造和赋值
    FFmpegWrapper(const FFmpegWrapper&) = delete;
    FFmpegWrapper& operator=(const FFmpegWrapper&) = delete;

    // 允许移动
    FFmpegWrapper(FFmpegWrapper&&) = default;
    FFmpegWrapper& operator=(FFmpegWrapper&&) = default;

    // 公共接口
    bool initialize(const EncoderConfig& config);

    bool encoderFrame(const FrameData& frameData);

    void finalize();

    // 状态查询接口
    int64_t getOutputFileSize() const;
    std::string getLastError() const { return lastError_; }
    bool isInitialized() const { return isInitialized_; }
    int64_t getEncodedFrameCount() const { return encodedFrameCount_; }

private:
    bool createOutputFile(const std::string& path);
    bool configureEncoder(const EncoderConfig& config);
    bool convertPixelFormat(const FrameData& frameData, AVFrame* dstFrame);
    bool writePacket(AVPacket* packet);
    void flushEncoder();
    void cleanup();

    AVPacketPtr packet_;
    SwsContextPtr swsContext_;
    AVFramePtr frame_;
    AVCodecContextPtr codecContext_;
    AVFormatContextPtr formatContext_;

    AVStream* videoStream_ = nullptr;

    bool isInitialized_ = false;
    std::string lastError_;
    std::string outputFilePath_;
    int64_t encodedFrameCount_ = 0;
    int width_ = 0;
    int height_ = 0;

    // 源像素格式（用于动态创建 SwsContext）
    PixelFormat srcPixelFormat_ = PixelFormat::UNKNOWN;

    // 帧缓冲区是否已分配（避免重复分配）
    bool frameBufferAllocated_ = false;
};

struct EncoderConfig {
    std::string outputFilePath;
    int width;
    int height;
    int fps;
    int bitrate;         // 码率
    int crf;             // 质量参数
    std::string preset;  // 编码预设
    std::string codec;   // 编码器名称
};

inline EncoderConfig defaultEncoderConfig() {
    EncoderConfig config;
    config.outputFilePath = "output.mp4";
    config.width = 1920;
    config.height = 1080;
    config.fps = 30;
    config.bitrate = 4000000;  // 4 Mbps
    config.crf = 23;           // 默认质量
    config.preset = "fast";    // 默认预设
    config.codec = "libx264";  // 默认编码器
    return config;
};