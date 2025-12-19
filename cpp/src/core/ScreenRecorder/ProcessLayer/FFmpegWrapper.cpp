
extern "C" {
#include <libavcodec/avcodec.h>

#include "libavcodec/codec.h"
#include "libavcodec/codec_id.h"
#include "libavcodec/packet.h"
#include "libavformat/avformat.h"
#include "libavformat/avio.h"
#include "libavutil/dict.h"
#include "libavutil/error.h"
#include "libavutil/frame.h"
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
            return AV_PIX_FMT_RGB24;
    }
}

FFmpegWrapper::FFmpegWrapper() = default;
FFmpegWrapper::~FFmpegWrapper() {
    finalize();
}

bool FFmpegWrapper::initialize(const EncoderConfig& config) {
    LOG_INFO("FFmpegWrapper initializing with config: " + config.outputFilePath);

    width_ = config.width;
    height_ = config.height;
    outputFilePath_ = config.outputFilePath;

    // 按照流程顺序执行初始化
    if (!step1_allocateFormatContext()) {
        return false;
    }

    if (!step2_openOutputFile(config.outputFilePath)) {
        return false;
    }

    if (!step3_createVideoStreamAndEncoder(config)) {
        return false;
    }

    if (!step4_writeFileHeader()) {
        return false;
    }

    // 准备帧和包的数据结构
    srcPixelFormat_ = PixelFormat::UNKNOWN;
    frameBufferAllocated_ = false;

    frame_.reset(av_frame_alloc());
    if (!frame_) {
        lastError_ = "Failed to allocate AVFrame";
        LOG_ERROR(lastError_);
        return false;
    }

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

    isInitialized_ = true;
    LOG_INFO("FFmpegWrapper initialized successfully");
    return true;
}

// 流程图步骤1：分配AVFormatContext
bool FFmpegWrapper::step1_allocateFormatContext() {
    AVFormatContext* fmtCtx = nullptr;
    int ret = avformat_alloc_output_context2(&fmtCtx, nullptr, nullptr, outputFilePath_.c_str());

    if (ret < 0 || !fmtCtx) {
        lastError_ = "Step 1 failed: Could not allocate AVFormatContext";
        LOG_ERROR(lastError_);
        return false;
    }

    formatContext_.reset(fmtCtx);
    LOG_INFO("Step 1 completed: AVFormatContext allocated");
    return true;
}

// 流程图步骤2：打开输出文件
bool FFmpegWrapper::step2_openOutputFile(const std::string& path) {
    if (!(formatContext_->oformat->flags & AVFMT_NOFILE)) {
        int ret = avio_open(&formatContext_->pb, path.c_str(), AVIO_FLAG_WRITE);
        if (ret < 0) {
            lastError_ = "Step 2 failed: Could not open output file: " + path;
            LOG_ERROR(lastError_);
            return false;
        }
    }

    LOG_INFO("Step 2 completed: Output file opened");
    return true;
}

// 流程图步骤3：创建视频流 + 配置编码器
bool FFmpegWrapper::step3_createVideoStreamAndEncoder(const EncoderConfig& config) {
    // 3.1 查找编码器
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
        lastError_ = "Step 3 failed: Could not find encoder";
        LOG_ERROR(lastError_);
        return false;
    }
    LOG_INFO("Using encoder: " + std::string(codec->name));

    // 3.2 创建视频流
    videoStream_ = avformat_new_stream(formatContext_.get(), codec);
    if (!videoStream_) {
        lastError_ = "Step 3 failed: Could not create video stream";
        LOG_ERROR(lastError_);
        return false;
    }

    // 3.3 分配编码器上下文
    codecContext_.reset(avcodec_alloc_context3(codec));
    if (!codecContext_) {
        lastError_ = "Step 3 failed: Could not allocate codec context";
        LOG_ERROR(lastError_);
        return false;
    }

    // 3.4 配置编码器参数
    codecContext_->width = config.width;
    codecContext_->height = config.height;
    codecContext_->pix_fmt = AV_PIX_FMT_YUV420P;
    codecContext_->bit_rate = config.bitrate;

    // 设置帧率和时间基（time_base 必须在 avcodec_open2 之前设置）
    codecContext_->time_base = AVRational{1, config.fps};
    codecContext_->framerate = AVRational{config.fps, 1};

    // 设置视频流的 time_base（libx264 在较新版本 FFmpeg 中要求此设置）
    videoStream_->time_base = codecContext_->time_base;

    codecContext_->gop_size = config.fps;
    codecContext_->max_b_frames = 2;

    // 3.5 打开编码器（并设置编码选项）
    AVDictionary* opts = nullptr;
    av_dict_set(&opts, "preset", config.preset.c_str(), 0);
    av_dict_set(&opts, "crf", std::to_string(config.crf).c_str(), 0);

    int ret = avcodec_open2(codecContext_.get(), codec, &opts);
    av_dict_free(&opts);  // 释放字典内存
    if (ret < 0) {
        lastError_ = "Step 3 failed: Could not open codec";
        LOG_ERROR(lastError_);
        return false;
    }

    // 3.6 将编码器参数复制到流
    ret = avcodec_parameters_from_context(videoStream_->codecpar, codecContext_.get());
    if (ret < 0) {
        lastError_ = "Step 3 failed: Could not copy codec parameters";
        LOG_ERROR(lastError_);
        return false;
    }

    LOG_INFO("Step 3 completed: Video stream and encoder configured");
    return true;
}

// 流程图步骤4：写入文件头
bool FFmpegWrapper::step4_writeFileHeader() {
    int ret = avformat_write_header(formatContext_.get(), nullptr);
    if (ret < 0) {
        lastError_ = "Step 4 failed: Could not write file header";
        LOG_ERROR(lastError_);
        return false;
    }

    LOG_INFO("Step 4 completed: File header written");
    return true;
}

// 流程图循环部分：编码帧 + 写入包
bool FFmpegWrapper::encoderFrame(const FrameData& frameData) {
    if (!isInitialized_) {
        lastError_ = "Encoder not initialized";
        LOG_ERROR(lastError_);
        return false;
    }

    // 步骤5：循环：编码帧 + 写入包
    if (!step5_encodeAndWriteFrame(frameData)) {
        return false;
    }

    encodedFrameCount_++;
    return true;
}

// 流程图步骤5：循环：编码帧 + 写入包
bool FFmpegWrapper::step5_encodeAndWriteFrame(const FrameData& frameData) {
    // 5.1 采集屏幕帧数据（由外部传入）
    // 5.2 转换原始帧到 YUV420P 格式
    if (!convertPixelFormat(frameData, frame_.get())) {
        lastError_ = "Step 5 failed: Failed to convert pixel format";
        LOG_ERROR(lastError_);
        return false;
    }

    frame_->pts = encodedFrameCount_;

    // 5.3 使用编码器编码帧
    int ret = avcodec_send_frame(codecContext_.get(), frame_.get());
    if (ret < 0) {
        lastError_ = "Step 5 failed: Error sending frame to encoder";
        LOG_ERROR(lastError_);
        return false;
    }

    // 5.4 获取压缩包（Packet）
    while (ret >= 0) {
        ret = avcodec_receive_packet(codecContext_.get(), packet_.get());
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
            break;
        } else if (ret < 0) {
            lastError_ = "Step 5 failed: Error receiving packet";
            LOG_ERROR(lastError_);
            return false;
        }

        // 5.5 封装（Mux）并写入文件
        if (!step6_muxAndWritePacket(packet_.get())) {
            lastError_ = "Step 5 failed: Failed to write packet";
            LOG_ERROR(lastError_);
            return false;
        }
        av_packet_unref(packet_.get());
    }

    return true;
}

// 流程图步骤6：刷新编码器
bool FFmpegWrapper::step6_flushEncoder() {
    LOG_INFO("Step 6: Flushing encoder...");

    int ret = avcodec_send_frame(codecContext_.get(), nullptr);
    if (ret < 0) {
        LOG_ERROR("Step 6 failed: Error sending flush frame");
        return false;
    }

    while (ret >= 0) {
        ret = avcodec_receive_packet(codecContext_.get(), packet_.get());
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
            break;
        } else if (ret < 0) {
            LOG_ERROR("Step 6 failed: Error receiving packet during flush");
            return false;
        }

        if (!step6_muxAndWritePacket(packet_.get())) {
            LOG_ERROR("Step 6 failed: Failed to write packet");
            return false;
        }
        av_packet_unref(packet_.get());
    }

    LOG_INFO("Step 6 completed: Encoder flushed");
    return true;
}

// 封装并写入包到文件（步骤5和步骤6共用）
bool FFmpegWrapper::step6_muxAndWritePacket(AVPacket* packet) {
    packet->stream_index = videoStream_->index;

    // 调整时间戳
    av_packet_rescale_ts(packet, codecContext_->time_base, videoStream_->time_base);

    // 写入文件
    int ret = av_interleaved_write_frame(formatContext_.get(), packet);
    if (ret < 0) {
        LOG_ERROR("Error writing packet to output file");
        return false;
    }
    return true;
}

// 流程图步骤7：写入文件尾 + 清理资源
void FFmpegWrapper::step7_writeTrailerAndCleanup() {
    LOG_INFO("Step 7: Writing trailer and cleaning up...");

    // 写入文件尾
    if (formatContext_) {
        av_write_trailer(formatContext_.get());
    }

    // 关闭文件
    if (formatContext_ && !(formatContext_->oformat->flags & AVFMT_NOFILE)) {
        avio_closep(&formatContext_->pb);
    }

    // 清理资源
    cleanup();

    LOG_INFO("Step 7 completed: Cleanup finished");
}

// 像素格式转换辅助函数
bool FFmpegWrapper::convertPixelFormat(const FrameData& frameData, AVFrame* dstFrame) {
    if (!swsContext_ || srcPixelFormat_ != frameData.format) {
        AVPixelFormat srcFmt = convertGdiPixelFormat(frameData.format);
        srcPixelFormat_ = frameData.format;

        swsContext_.reset(sws_getContext(width_, height_, srcFmt, width_, height_,
                                         AV_PIX_FMT_YUV420P, SWS_BILINEAR, nullptr, nullptr,
                                         nullptr));

        if (!swsContext_) {
            LOG_ERROR("Failed to create SwsContext");
            return false;
        }
    }

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
            bytesPerPixel = 4;
    }

    uint8_t* srcData[4] = {frameData.data, nullptr, nullptr, nullptr};
    int srcLinesize[4] = {frameData.width * bytesPerPixel, 0, 0, 0};

    int ret = av_frame_make_writable(dstFrame);
    if (ret < 0) {
        LOG_ERROR("Could not make frame writable");
        return false;
    }

    int scaleRet = sws_scale(swsContext_.get(), srcData, srcLinesize, 0, height_, dstFrame->data,
                             dstFrame->linesize);
    if (scaleRet <= 0) {
        LOG_ERROR("Failed to scale frame");
        return false;
    }
    return true;
}

void FFmpegWrapper::finalize() {
    if (!isInitialized_) {
        return;
    }

    step6_flushEncoder();
    step7_writeTrailerAndCleanup();
    isInitialized_ = false;
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