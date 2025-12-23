
#include <mutex>

#include "AudioData.h"

extern "C" {
#include <libavcodec/avcodec.h>

#include "libavcodec/codec.h"
#include "libavcodec/codec_id.h"
#include "libavcodec/packet.h"
#include "libavformat/avformat.h"
#include "libavformat/avio.h"
#include "libavutil/audio_fifo.h"
#include "libavutil/channel_layout.h"
#include "libavutil/dict.h"
#include "libavutil/error.h"
#include "libavutil/frame.h"
#include "libavutil/mem.h"
#include "libavutil/pixfmt.h"
#include "libavutil/rational.h"
#include "libavutil/samplefmt.h"
#include "libswresample/swresample.h"
#include "libswscale/swscale.h"
}

#include <cerrno>
#include <cstdint>
#include <string>

#include "FFmpegWrapper.h"
#include "Log.h"
#include "VideoGrabber.h"


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

    if (config.enableAudio) {
        if (!step3_createAudioStreamAndEncoder(config)) {
            return false;
        }
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

bool FFmpegWrapper::step3_createAudioStreamAndEncoder(const EncoderConfig& config) {
    // 3.1 查找音频编码器
    const AVCodec* codec = avcodec_find_encoder_by_name(config.audioCodec.c_str());
    if (!codec) {
        codec = avcodec_find_encoder(AV_CODEC_ID_AAC);
    }
    if (!codec) {
        lastError_ = "Step 3 (Audio) failed: Could not find audio encoder";
        LOG_ERROR(lastError_);
        return false;
    }

    // 3.2 创建音频流
    audioStream_ = avformat_new_stream(formatContext_.get(), codec);
    if (!audioStream_) {
        lastError_ = "Step 3 (Audio) failed: Could not create audio stream";
        LOG_ERROR(lastError_);
        return false;
    }

    // 3.3 分配编码器上下文
    audioCodecContext_.reset(avcodec_alloc_context3(codec));
    if (!audioCodecContext_) {
        lastError_ = "Step 3 (Audio) failed: Could not allocate audio codec context";
        LOG_ERROR(lastError_);
        return false;
    }

    // 3.4 配置音频编码器参数
    audioCodecContext_->sample_fmt =
        codec->sample_fmts ? codec->sample_fmts[0] : AV_SAMPLE_FMT_FLTP;
    audioCodecContext_->bit_rate = config.audioBitrate;
    audioCodecContext_->sample_rate = config.audioSampleRate;

    // 设置通道布局
    av_channel_layout_default(&audioCodecContext_->ch_layout, config.audioChannels);

    audioCodecContext_->time_base = AVRational{1, config.audioSampleRate};
    audioStream_->time_base = audioCodecContext_->time_base;

    // 3.5 打开编码器
    int ret = avcodec_open2(audioCodecContext_.get(), codec, nullptr);
    if (ret < 0) {
        lastError_ = "Step 3 (Audio) failed: Could not open audio codec";
        LOG_ERROR(lastError_);
        return false;
    }

    // 3.6 将编码器参数复制到流
    ret = avcodec_parameters_from_context(audioStream_->codecpar, audioCodecContext_.get());
    if (ret < 0) {
        lastError_ = "Step 3 (Audio) failed: Could not copy audio codec parameters";
        LOG_ERROR(lastError_);
        return false;
    }

    // 3.7 准备音频帧和包
    audioFrame_.reset(av_frame_alloc());
    audioFrame_->format = audioCodecContext_->sample_fmt;
    audioFrame_->nb_samples = audioCodecContext_->frame_size;
    av_channel_layout_copy(&audioFrame_->ch_layout, &audioCodecContext_->ch_layout);
    audioFrame_->sample_rate = audioCodecContext_->sample_rate;

    if (av_frame_get_buffer(audioFrame_.get(), 0) < 0) {
        lastError_ = "Step 3 (Audio) failed: Could not allocate audio frame buffer";
        LOG_ERROR(lastError_);
        return false;
    }

    audioPacket_.reset(av_packet_alloc());

    // 3.8 初始化音频 FIFO
    audioFifo_.reset(av_audio_fifo_alloc(audioCodecContext_->sample_fmt,
                                         audioCodecContext_->ch_layout.nb_channels,
                                         audioCodecContext_->sample_rate));  // 初始容量设为 1 秒

    LOG_INFO("Step 3 completed: Audio stream and encoder configured");
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
        if (!step6_muxAndWritePacket(packet_.get(), videoStream_)) {
            lastError_ = "Step 5 failed: Failed to write packet";
            LOG_ERROR(lastError_);
            return false;
        }
        av_packet_unref(packet_.get());
    }

    return true;
}

bool FFmpegWrapper::encodeAudioFrame(const AudioData& audioData) {
    if (!isInitialized_ || !audioStream_ || !audioFifo_) {
        return false;
    }

    // 1. 初始化或更新重采样器
    AVSampleFormat inSampleFmt =
        (audioData.bitsPerSample == 32) ? AV_SAMPLE_FMT_FLT : AV_SAMPLE_FMT_S16;
    AVChannelLayout inChannelLayout;
    av_channel_layout_default(&inChannelLayout, audioData.channels);

    if (!swrContext_) {
        SwrContext* swr = nullptr;
        swr_alloc_set_opts2(&swr, &audioCodecContext_->ch_layout, audioCodecContext_->sample_fmt,
                            audioCodecContext_->sample_rate, &inChannelLayout, inSampleFmt,
                            audioData.sampleRate, 0, nullptr);
        swrContext_.reset(swr);
        swr_init(swrContext_.get());
    }

    // 2. 重采样并推入 FIFO
    uint8_t* inData[1] = {const_cast<uint8_t*>(audioData.data.data())};
    int inSamples = audioData.samplesPerChannel;

    // 计算输出样本数并分配临时缓冲区
    int maxOutSamples = swr_get_out_samples(swrContext_.get(), inSamples);
    uint8_t** outData = nullptr;
    int outLinesize;
    av_samples_alloc_array_and_samples(&outData, &outLinesize,
                                       audioCodecContext_->ch_layout.nb_channels, maxOutSamples,
                                       audioCodecContext_->sample_fmt, 0);

    int outSamples =
        swr_convert(swrContext_.get(), outData, maxOutSamples, (const uint8_t**)inData, inSamples);
    if (outSamples > 0) {
        av_audio_fifo_write(audioFifo_.get(), (void**)outData, outSamples);
    }

    if (outData) {
        av_freep(&outData[0]);
        av_freep(&outData);
    }

    // 3. 从 FIFO 中提取足够一帧的数据进行编码
    int frameSize = audioCodecContext_->frame_size;
    while (av_audio_fifo_size(audioFifo_.get()) >= frameSize) {
        int ret = av_frame_make_writable(audioFrame_.get());
        if (ret < 0)
            break;

        if (av_audio_fifo_read(audioFifo_.get(), (void**)audioFrame_->data, frameSize) <
            frameSize) {
            break;
        }

        audioFrame_->pts = audioSamplesCount_;
        audioSamplesCount_ += frameSize;

        ret = avcodec_send_frame(audioCodecContext_.get(), audioFrame_.get());
        if (ret < 0)
            break;

        while (ret >= 0) {
            ret = avcodec_receive_packet(audioCodecContext_.get(), audioPacket_.get());
            if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
                break;
            if (ret < 0)
                return false;

            if (!step6_muxAndWritePacket(audioPacket_.get(), audioStream_)) {
                return false;
            }
            av_packet_unref(audioPacket_.get());
        }
    }

    return true;
}

// 流程图步骤6：刷新编码器
bool FFmpegWrapper::step6_flushEncoder() {
    std::lock_guard<std::mutex> lock(muxerMutex_);
    LOG_INFO("Step 6: Flushing encoder...");

    // 刷新视频
    if (codecContext_) {
        avcodec_send_frame(codecContext_.get(), nullptr);
        while (avcodec_receive_packet(codecContext_.get(), packet_.get()) >= 0) {
            // 注意：这里内部调用不需要再加锁，但因为我们已经拿到了锁，所以没问题
            // 为了避免死锁，我们可以直接写逻辑或者使用递归锁，但这里简单处理
            packet_->stream_index = videoStream_->index;
            av_packet_rescale_ts(packet_.get(), codecContext_->time_base, videoStream_->time_base);
            av_interleaved_write_frame(formatContext_.get(), packet_.get());
            av_packet_unref(packet_.get());
        }
    }

    // 刷新音频
    if (audioCodecContext_) {
        avcodec_send_frame(audioCodecContext_.get(), nullptr);
        while (avcodec_receive_packet(audioCodecContext_.get(), audioPacket_.get()) >= 0) {
            audioPacket_->stream_index = audioStream_->index;
            av_packet_rescale_ts(audioPacket_.get(), audioCodecContext_->time_base,
                                 audioStream_->time_base);
            av_interleaved_write_frame(formatContext_.get(), audioPacket_.get());
            av_packet_unref(audioPacket_.get());
        }
    }

    LOG_INFO("Step 6 completed: Encoder flushed");
    return true;
}

// 封装并写入包到文件（步骤5和步骤6共用）
bool FFmpegWrapper::step6_muxAndWritePacket(AVPacket* packet, AVStream* stream) {
    std::lock_guard<std::mutex> lock(muxerMutex_);
    packet->stream_index = stream->index;

    // 调整时间戳
    AVCodecContext* ctx = (stream == videoStream_) ? codecContext_.get() : audioCodecContext_.get();
    av_packet_rescale_ts(packet, ctx->time_base, stream->time_base);

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

    audioCodecContext_.reset();
    audioFrame_.reset();
    audioPacket_.reset();
    swrContext_.reset();
    audioFifo_.reset();

    formatContext_.reset();
}

int64_t FFmpegWrapper::getOutputFileSize() const {
    if (!isInitialized_ || !formatContext_->pb) {
        return 0;
    }
    return avio_size(formatContext_->pb);
}