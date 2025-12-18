
extern "C" {
#include <libavcodec/avcodec.h>

#include "libavcodec/codec.h"
#include "libavcodec/codec_id.h"
#include "libavcodec/packet.h"
#include "libavformat/avformat.h"
#include "libavformat/avio.h"
#include "libavutil/error.h"
#include "libavutil/frame.h"
#include "libavutil/opt.h"
#include "libavutil/pixfmt.h"
#include "libavutil/rational.h"
#include "libswscale/swscale.h"
}

#include <cerrno>
#include <cstdint>
#include <string>

#include "FFmpegWrapper.h"
#include "IScreenGrabber.h"
#include "Log.h"

// 辅助函数：将GDI PixelFormat转换为FFmpeg AVPixelFormat
static AVPixelFormat convertGdiPixelFormat(PixelFormat format) {
    switch (format) {
        case PixelFormat::BGRA:
            return AV_PIX_FMT_BGRA;
        case PixelFormat::RGBA:
            return AV_PIX_FMT_RGBA;
        case PixelFormat::RGB24:
            return AV_PIX_FMT_RGB24;
        default:
            return AV_PIX_FMT_RGB24;  // 默认RGB24
    }
}

FFmpegWrapper::FFmpegWrapper() = default;
FFmpegWrapper::~FFmpegWrapper() {
    finalize();
}

bool FFmpegWrapper::initialize(const EncoderConfig& config) {
    LOG_INFO("FFmpegWrapper initializing with config: " + config.outputFilePath);

    // 使用传入的配置（而非忽略）
    width_ = config.width;
    height_ = config.height;
    outputFilePath_ = config.outputFilePath;

    if (!createOutputFile(config.outputFilePath)) {
        lastError_ = "Failed to create output file";
        LOG_ERROR(lastError_);
        return false;
    }

    if (!configureEncoder(config)) {
        lastError_ = "Failed to configure encoder";
        LOG_ERROR(lastError_);
        return false;
    }

    // SwsContext 延迟初始化：在第一次 encoderFrame 时根据实际像素格式创建
    // 这样可以正确处理 BGRA/RGBA/RGB24 等不同格式
    srcPixelFormat_ = PixelFormat::UNKNOWN;
    frameBufferAllocated_ = false;

    frame_.reset(av_frame_alloc());
    if (!frame_) {
        lastError_ = "Failed to allocate AVFrame";
        LOG_ERROR(lastError_);
        return false;
    }

    // 预分配目标帧缓冲区（YUV420P 格式）
    frame_->format = AV_PIX_FMT_YUV420P;
    frame_->width = width_;
    frame_->height = height_;
    int ret = av_frame_get_buffer(frame_.get(), 32);
    if (ret < 0) {
        lastError_ = "Failed to allocate frame buffer";
        LOG_ERROR(lastError_);
        return false;
    }
    frameBufferAllocated_ = true;

    packet_.reset(av_packet_alloc());
    if (!packet_) {
        lastError_ = "Failed to allocate AVPacket";
        LOG_ERROR(lastError_);
        return false;
    }

    ret = avformat_write_header(formatContext_.get(), nullptr);
    if (ret < 0) {
        lastError_ = "Failed to write file header";
        LOG_ERROR(lastError_);
        return false;
    }

    isInitialized_ = true;
    LOG_INFO("FFmpegWrapper initialized successfully");
    return true;
}

bool FFmpegWrapper::createOutputFile(const std::string& path) {
    // 分配AVFormatContext
    AVFormatContext* fmtCtx = nullptr;
    int ret = avformat_alloc_output_context2(&fmtCtx, nullptr, nullptr, path.c_str());
    if (ret < 0 || !fmtCtx) {
        LOG_ERROR("Could not allocate output format context");
        return false;
    }
    formatContext_.reset(fmtCtx);

    // 打开输出文件
    if (!(formatContext_->oformat->flags & AVFMT_NOFILE)) {
        ret = avio_open(&formatContext_->pb, path.c_str(), AVIO_FLAG_WRITE);
        if (ret < 0) {
            LOG_ERROR("Could not open output file: " + path);
            return false;
        }
    }
    outputFilePath_ = path;
    return true;
}

bool FFmpegWrapper::configureEncoder(const EncoderConfig& config) {
    // 查找编码器：优先使用配置中指定的编码器，否则使用默认 H.264
    const AVCodec* codec = nullptr;
    if (!config.codec.empty()) {
        codec = avcodec_find_encoder_by_name(config.codec.c_str());
        if (!codec) {
            LOG_WARN("Codec '" + config.codec + "' not found, falling back to H.264");
        }
    }
    if (!codec) {
        codec = avcodec_find_encoder(AV_CODEC_ID_H264);
    }
    if (!codec) {
        LOG_ERROR("Could not find any suitable encoder");
        return false;
    }
    LOG_INFO("Using encoder: " + std::string(codec->name));

    // 创建流
    videoStream_ = avformat_new_stream(formatContext_.get(), codec);
    if (!videoStream_) {
        LOG_ERROR("Could not create new stream");
        return false;
    }

    codecContext_.reset(avcodec_alloc_context3(codec));
    if (!codecContext_) {
        LOG_ERROR("Could not allocate codec context");
        return false;
    }
    codecContext_->width = config.width;
    codecContext_->height = config.height;
    codecContext_->time_base = AVRational{1, config.fps};
    codecContext_->framerate = AVRational{config.fps, 1};
    codecContext_->gop_size = config.fps;
    codecContext_->max_b_frames = 2;
    codecContext_->pix_fmt = AV_PIX_FMT_YUV420P;
    codecContext_->bit_rate = config.bitrate;

    // 设置编码器参数
    av_opt_set(codecContext_->priv_data, "preset", config.preset.c_str(), 0);
    av_opt_set(codecContext_->priv_data, "crf", std::to_string(config.crf).c_str(), 0);

    // 打开编码器
    int ret = avcodec_open2(codecContext_.get(), codec, nullptr);
    if (ret < 0) {
        LOG_ERROR("Could not open codec");
        return false;
    }

    // 拷贝参数到流
    ret = avcodec_parameters_from_context(videoStream_->codecpar, codecContext_.get());
    if (ret < 0) {
        LOG_ERROR("Could not copy codec parameters to stream");
        return false;
    }

    return true;
}

bool FFmpegWrapper::encoderFrame(const FrameData& frameData) {
    if (!isInitialized_) {
        lastError_ = "Encoder not initialized";
        LOG_ERROR(lastError_);
        return false;
    }

    // 转换像素格式
    if (!convertPixelFormat(frameData, frame_.get())) {
        lastError_ = "Failed to convert pixel format";
        LOG_ERROR(lastError_);
        return false;
    }

    frame_->pts = encodedFrameCount_;

    // 发送帧到编码器
    int ret = avcodec_send_frame(codecContext_.get(), frame_.get());
    if (ret < 0) {
        lastError_ = "Error sending frame to encoder";
        LOG_ERROR(lastError_);
        return false;
    }

    // 接收编码后的数据包
    while (ret >= 0) {
        ret = avcodec_receive_packet(codecContext_.get(), packet_.get());
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
            break;
        } else if (ret < 0) {
            lastError_ = "Error receiving packet from encoder";
            LOG_ERROR(lastError_);
            return false;
        }

        // 写入数据包到输出文件
        if (!writePacket(packet_.get())) {
            lastError_ = "Failed to write packet";
            LOG_ERROR(lastError_);
            return false;
        }
        av_packet_unref(packet_.get());
    }

    encodedFrameCount_++;
    return true;
}

bool FFmpegWrapper::convertPixelFormat(const FrameData& frameData, AVFrame* dstFrame) {
    // 检查是否需要（重新）创建 SwsContext
    // 情况：首次调用 或 源像素格式变化
    if (!swsContext_ || srcPixelFormat_ != frameData.format) {
        AVPixelFormat srcFmt = convertGdiPixelFormat(frameData.format);
        srcPixelFormat_ = frameData.format;

        swsContext_.reset(sws_getContext(width_, height_, srcFmt, width_, height_,
                                         AV_PIX_FMT_YUV420P, SWS_BILINEAR, nullptr, nullptr,
                                         nullptr));

        if (!swsContext_) {
            LOG_ERROR("Failed to create SwsContext for format conversion");
            return false;
        }
        LOG_INFO("SwsContext created for pixel format: " +
                 std::to_string(static_cast<int>(frameData.format)));
    }

    // 根据源数据格式确定字节数和行大小
    int bytesPerPixel = 0;
    switch (frameData.format) {
        case PixelFormat::BGRA:
        case PixelFormat::RGBA:
            bytesPerPixel = 4;
            break;
        case PixelFormat::RGB24:
            bytesPerPixel = 3;
            break;
        default:
            bytesPerPixel = 4;  // 默认假设 BGRA
    }

    // 设置源帧数据
    uint8_t* srcData[4] = {frameData.data, nullptr, nullptr, nullptr};
    int srcLinesize[4] = {frameData.width * bytesPerPixel, 0, 0, 0};

    // 使用预分配的目标帧缓冲区（已在 initialize 中分配）
    // 确保帧可写（如果被引用需要重新分配）
    int ret = av_frame_make_writable(dstFrame);
    if (ret < 0) {
        LOG_ERROR("Could not make frame writable");
        return false;
    }

    // 执行像素格式转换
    int scaleRet = sws_scale(swsContext_.get(), srcData, srcLinesize, 0, height_, dstFrame->data,
                             dstFrame->linesize);
    if (scaleRet <= 0) {
        LOG_ERROR("Failed to scale frame: " + std::to_string(scaleRet));
        return false;
    }
    return true;
}

bool FFmpegWrapper::writePacket(AVPacket* packet) {
    packet->stream_index = videoStream_->index;

    // 调整时间戳到流的时间基
    av_packet_rescale_ts(packet, codecContext_->time_base, videoStream_->time_base);

    int ret = av_interleaved_write_frame(formatContext_.get(), packet);
    if (ret < 0) {
        LOG_ERROR("Error writing packet to output file");
        return false;
    }
    return true;
}

void FFmpegWrapper::finalize() {
    if (!isInitialized_) {
        return;
    }

    flushEncoder();

    av_write_trailer(formatContext_.get());

    if (formatContext_ && !(formatContext_->oformat->flags & AVFMT_NOFILE)) {
        avio_closep(&formatContext_->pb);
    }

    cleanup();
    isInitialized_ = false;
}

void FFmpegWrapper::flushEncoder() {
    int ret = avcodec_send_frame(codecContext_.get(), nullptr);
    if (ret < 0) {
        LOG_ERROR("Error sending flush frame to encoder");
        return;
    }

    while (ret >= 0) {
        ret = avcodec_receive_packet(codecContext_.get(), packet_.get());
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
            break;
        } else if (ret < 0) {
            LOG_ERROR("Error receiving packet during flush");
            return;
        }

        if (!writePacket(packet_.get())) {
            LOG_ERROR("Failed to write packet during flush");
            return;
        }
        av_packet_unref(packet_.get());
    }
}

void FFmpegWrapper::cleanup() {
    packet_.reset();
    frame_.reset();
    swsContext_.reset();
    codecContext_.reset();
    formatContext_.reset();
}

int64_t FFmpegWrapper::getOutputFileSize() const {
    if (!isInitialized_ || !formatContext_->pb) {
        return 0;
    }
    return avio_size(formatContext_->pb);
}
