#pragma once

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/audio_fifo.h>
#include <libavutil/imgutils.h>
#include <libavutil/opt.h>
#include <libswresample/swresample.h>
#include <libswscale/swscale.h>

#include "libavcodec/packet.h"
#include "libavformat/avio.h"
#include "libavutil/frame.h"
}

#include <cstdint>
#include <memory>
#include <mutex>
#include <string>

#include "AudioData.h"
#include "VideoGrabber.h"

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

struct SwrContextDeleter {
    void operator()(SwrContext* ptr) noexcept {
        if (ptr) {
            swr_free(&ptr);
        }
    }
};

struct AVAudioFifoDeleter {
    void operator()(AVAudioFifo* ptr) noexcept {
        if (ptr) {
            av_audio_fifo_free(ptr);
        }
    }
};

using AVFormatContextPtr = std::unique_ptr<AVFormatContext, AVFormatContextDeleter>;
using AVCodecContextPtr = std::unique_ptr<AVCodecContext, AVCodecContextDeleter>;
using AVFramePtr = std::unique_ptr<AVFrame, AVFrameDeleter>;
using AVPacketPtr = std::unique_ptr<AVPacket, AVPacketDeleter>;
using SwsContextPtr = std::unique_ptr<SwsContext, SwsContextDeleter>;
using SwrContextPtr = std::unique_ptr<SwrContext, SwrContextDeleter>;
using AVAudioFifoPtr = std::unique_ptr<AVAudioFifo, AVAudioFifoDeleter>;

class FFmpegWrapper {
public:
    FFmpegWrapper();
    ~FFmpegWrapper();

    // 禁止拷贝构造和赋值
    FFmpegWrapper(const FFmpegWrapper&) = delete;
    FFmpegWrapper& operator=(const FFmpegWrapper&) = delete;

    // 由于包含 std::mutex，禁止移动
    FFmpegWrapper(FFmpegWrapper&&) = delete;
    FFmpegWrapper& operator=(FFmpegWrapper&&) = delete;

    // 公共接口
    bool initialize(const EncoderConfig& config);

    bool encoderFrame(const FrameData& frameData);
    bool encodeAudioFrame(const AudioData& audioData);

    void finalize();

    // 状态查询接口
    int64_t getOutputFileSize() const;
    std::string getLastError() const { return lastError_; }
    bool isInitialized() const { return isInitialized_; }
    int64_t getEncodedFrameCount() const { return encodedFrameCount_; }

private:
    // FFmpeg 视频编码流程的7个步骤
    bool step1_allocateFormatContext();
    bool step2_openOutputFile(const std::string& path);
    bool step3_createVideoStreamAndEncoder(const EncoderConfig& config);
    bool step3_createAudioStreamAndEncoder(const EncoderConfig& config);
    bool step4_writeFileHeader();
    bool step5_encodeAndWriteFrame(const FrameData& frameData);
    bool step6_flushEncoder();
    bool step6_muxAndWritePacket(AVPacket* packet, AVStream* stream);
    void step7_writeTrailerAndCleanup();

    // 辅助函数
    bool convertPixelFormat(const FrameData& frameData, AVFrame* dstFrame);
    void cleanup();

    AVPacketPtr packet_;
    SwsContextPtr swsContext_;
    AVFramePtr frame_;
    AVCodecContextPtr codecContext_;
    AVFormatContextPtr formatContext_;

    // 音频相关
    AVCodecContextPtr audioCodecContext_;
    AVFramePtr audioFrame_;
    AVPacketPtr audioPacket_;
    AVStream* audioStream_ = nullptr;
    SwrContextPtr swrContext_;
    AVAudioFifoPtr audioFifo_;

    AVStream* videoStream_ = nullptr;

    bool isInitialized_ = false;
    std::string lastError_;
    std::string outputFilePath_;
    int64_t encodedFrameCount_ = 0;
    int64_t audioSamplesCount_ = 0;
    int width_ = 0;
    int height_ = 0;

    // 源像素格式（用于动态创建 SwsContext）
    PixelFormat srcPixelFormat_ = PixelFormat::UNKNOWN;

    // 帧缓冲区是否已分配（避免重复分配）
    bool frameBufferAllocated_ = false;

    mutable std::mutex muxerMutex_;
};

struct EncoderConfig {
    std::string outputFilePath;

    // 视频参数
    int width;
    int height;
    int fps;
    int bitrate;         // 视频码率
    int crf;             // 质量参数
    std::string preset;  // 编码预设
    std::string codec;   // 视频编码器名称

    // 音频参数
    bool enableAudio = true;
    int audioSampleRate = 48000;
    int audioChannels = 2;
    int audioBitrate = 128000;  // 128 kbps
    std::string audioCodec = "aac";
};

// 默认配置（使用默认分辨率）
inline EncoderConfig defaultEncoderConfig(int width = 1920, int height = 1080) {
    EncoderConfig config;
    config.outputFilePath = "output.mp4";
    config.width = width;
    config.height = height;
    config.fps = 30;
    config.bitrate = 4000000;  // 4 Mbps
    config.crf = 23;           // 默认质量
    config.preset = "fast";    // 默认预设
    config.codec = "libx264";  // 默认编码器

    // 音频默认值
    config.enableAudio = true;
    config.audioSampleRate = 48000;
    config.audioChannels = 2;
    config.audioBitrate = 128000;
    config.audioCodec = "aac";

    return config;
}

// 从屏幕采集器获取分辨率的配置
inline EncoderConfig encoderConfigFromGrabber(const VideoGrabber* grabber) {
    if (!grabber) {
        return defaultEncoderConfig();  // 回退到默认配置
    }
    EncoderConfig config;
    config.outputFilePath = "output.mp4";
    config.width = grabber->getWidth();
    config.height = grabber->getHeight();
    config.fps = grabber->getFps() > 0 ? grabber->getFps() : 30;  // 默认为 30 FPS
    config.bitrate = 4000000;                                     // 4 Mbps
    config.crf = 23;                                              // 默认质量
    config.preset = "fast";                                       // 默认预设
    config.codec = "libx264";                                     // 默认编码器

    // 音频默认值
    config.enableAudio = true;
    config.audioSampleRate = 48000;
    config.audioChannels = 2;
    config.audioBitrate = 128000;
    config.audioCodec = "aac";

    return config;
};