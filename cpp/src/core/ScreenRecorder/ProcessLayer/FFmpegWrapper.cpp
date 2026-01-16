
#include <cerrno>
#include <cstddef>
#include <cstdint>
#include <mutex>
#include <string>

#include "AudioData.h"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswresample/swresample.h>
#include <libswscale/swscale.h>

#include "libavcodec/codec.h"
#include "libavcodec/codec_id.h"
#include "libavcodec/packet.h"
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

}

#include "FFmpegWrapper.h"
#include "Log.h"
#include "VideoGrabber.h"

namespace {

constexpr int FRAME_BUFFER_ALIGNMENT = 32;
constexpr int MAX_B_FRAMES = 0;  // Disabled for keyframe video compatibility

AVPixelFormat convertGdiPixelFormat(PixelFormat format) {
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

int getBytesPerPixel(PixelFormat format) {
    switch (format) {
        case PixelFormat::BGRA:
        case PixelFormat::RGBA:
            return 4;
        case PixelFormat::RGB24:
            return 3;
        default:
            return 4;
    }
}

}  // namespace

FFmpegWrapper::FFmpegWrapper() = default;

FFmpegWrapper::~FFmpegWrapper() {
    finalize();
}

bool FFmpegWrapper::initialize(const EncoderConfig& config) {
    LOG_INFO("FFmpegWrapper initializing with config: " + config.video.outputFilePath);

    width_ = config.video.width;
    height_ = config.video.height;
    outputFilePath_ = config.video.outputFilePath;

    if (!step1_allocateFormatContext()) {
        return false;
    }

    if (!step2_openOutputFile(config.video.outputFilePath)) {
        return false;
    }

    if (!step3_createVideoStreamAndEncoder(config)) {
        return false;
    }

    if (config.audio.enabled) {
        if (!step3_createAudioStreamAndEncoder(config)) {
            return false;
        }
    }

    if (!step4_writeFileHeader()) {
        return false;
    }

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

    int ret = av_frame_get_buffer(frame_.get(), FRAME_BUFFER_ALIGNMENT);
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

bool FFmpegWrapper::step1_allocateFormatContext() {
    AVFormatContext* fmtCtx = nullptr;
    int ret = avformat_alloc_output_context2(&fmtCtx, nullptr, nullptr, outputFilePath_.c_str());

    if (ret < 0 || !fmtCtx) {
        lastError_ = "Could not allocate AVFormatContext";
        LOG_ERROR(lastError_);
        return false;
    }

    formatContext_.reset(fmtCtx);
    LOG_INFO("AVFormatContext allocated");
    return true;
}

bool FFmpegWrapper::step2_openOutputFile(const std::string& path) {
    if (!(formatContext_->oformat->flags & AVFMT_NOFILE)) {
        int ret = avio_open(&formatContext_->pb, path.c_str(), AVIO_FLAG_WRITE);
        if (ret < 0) {
            lastError_ = "Could not open output file: " + path;
            LOG_ERROR(lastError_);
            return false;
        }
    }

    LOG_INFO("Output file opened");
    return true;
}

bool FFmpegWrapper::step3_createVideoStreamAndEncoder(const EncoderConfig& config) {
    const AVCodec* codec = nullptr;

    if (!config.video.codec.empty()) {
        codec = avcodec_find_encoder_by_name(config.video.codec.c_str());
        if (!codec) {
            LOG_WARN("Codec '" + config.video.codec + "' not found, falling back to H.264");
        }
    }

    if (!codec) {
        codec = avcodec_find_encoder(AV_CODEC_ID_H264);
    }

    if (!codec) {
        lastError_ = "Could not find encoder";
        LOG_ERROR(lastError_);
        return false;
    }

    LOG_INFO("Using encoder: " + std::string(codec->name));

    videoStream_ = avformat_new_stream(formatContext_.get(), codec);
    if (!videoStream_) {
        lastError_ = "Could not create video stream";
        LOG_ERROR(lastError_);
        return false;
    }

    codecContext_.reset(avcodec_alloc_context3(codec));
    if (!codecContext_) {
        lastError_ = "Could not allocate codec context";
        LOG_ERROR(lastError_);
        return false;
    }

    codecContext_->width = config.video.width;
    codecContext_->height = config.video.height;
    codecContext_->pix_fmt = AV_PIX_FMT_YUV420P;
    codecContext_->bit_rate = config.video.bitrate;
    codecContext_->time_base = AVRational{1, config.video.fps};
    codecContext_->framerate = AVRational{config.video.fps, 1};
    videoStream_->time_base = codecContext_->time_base;
    codecContext_->gop_size = config.video.fps;
    codecContext_->max_b_frames = MAX_B_FRAMES;

    AVDictionary* opts = nullptr;
    av_dict_set(&opts, "preset", config.video.preset.c_str(), 0);
    av_dict_set(&opts, "crf", std::to_string(config.video.crf).c_str(), 0);

    int ret = avcodec_open2(codecContext_.get(), codec, &opts);
    av_dict_free(&opts);

    if (ret < 0) {
        lastError_ = "Could not open codec";
        LOG_ERROR(lastError_);
        return false;
    }

    ret = avcodec_parameters_from_context(videoStream_->codecpar, codecContext_.get());
    if (ret < 0) {
        lastError_ = "Could not copy codec parameters";
        LOG_ERROR(lastError_);
        return false;
    }

    LOG_INFO("Video stream and encoder configured");
    return true;
}

bool FFmpegWrapper::step3_createAudioStreamAndEncoder(const EncoderConfig& config) {
    const AVCodec* codec = avcodec_find_encoder_by_name(config.audio.codec.c_str());
    if (!codec) {
        codec = avcodec_find_encoder(AV_CODEC_ID_AAC);
    }

    if (!codec) {
        lastError_ = "Could not find audio encoder";
        LOG_ERROR(lastError_);
        return false;
    }

    audioStream_ = avformat_new_stream(formatContext_.get(), codec);
    if (!audioStream_) {
        lastError_ = "Could not create audio stream";
        LOG_ERROR(lastError_);
        return false;
    }

    audioCodecContext_.reset(avcodec_alloc_context3(codec));
    if (!audioCodecContext_) {
        lastError_ = "Could not allocate audio codec context";
        LOG_ERROR(lastError_);
        return false;
    }

    audioCodecContext_->sample_fmt =
        codec->sample_fmts ? codec->sample_fmts[0] : AV_SAMPLE_FMT_FLTP;
    audioCodecContext_->bit_rate = config.audio.bitrate;
    audioCodecContext_->sample_rate = config.audio.sampleRate;

    av_channel_layout_default(&audioCodecContext_->ch_layout, config.audio.channels);

    audioCodecContext_->time_base = AVRational{1, config.audio.sampleRate};
    audioStream_->time_base = audioCodecContext_->time_base;

    int ret = avcodec_open2(audioCodecContext_.get(), codec, nullptr);
    if (ret < 0) {
        lastError_ = "Could not open audio codec";
        LOG_ERROR(lastError_);
        return false;
    }

    ret = avcodec_parameters_from_context(audioStream_->codecpar, audioCodecContext_.get());
    if (ret < 0) {
        lastError_ = "Could not copy audio codec parameters";
        LOG_ERROR(lastError_);
        return false;
    }

    audioFrame_.reset(av_frame_alloc());
    audioFrame_->format = audioCodecContext_->sample_fmt;
    audioFrame_->nb_samples = audioCodecContext_->frame_size;
    av_channel_layout_copy(&audioFrame_->ch_layout, &audioCodecContext_->ch_layout);
    audioFrame_->sample_rate = audioCodecContext_->sample_rate;

    if (av_frame_get_buffer(audioFrame_.get(), 0) < 0) {
        lastError_ = "Could not allocate audio frame buffer";
        LOG_ERROR(lastError_);
        return false;
    }

    audioPacket_.reset(av_packet_alloc());

    audioFifo_.reset(av_audio_fifo_alloc(audioCodecContext_->sample_fmt,
                                         audioCodecContext_->ch_layout.nb_channels,
                                         audioCodecContext_->sample_rate));

    LOG_INFO("Audio stream and encoder configured");
    return true;
}

bool FFmpegWrapper::step4_writeFileHeader() {
    int ret = avformat_write_header(formatContext_.get(), nullptr);
    if (ret < 0) {
        lastError_ = "Could not write file header";
        LOG_ERROR(lastError_);
        return false;
    }

    LOG_INFO("File header written");
    return true;
}

bool FFmpegWrapper::encoderFrame(const FrameData& frameData) {
    if (!isInitialized_) {
        lastError_ = "Encoder not initialized";
        LOG_ERROR(lastError_);
        return false;
    }

    if (!step5_encodeAndWriteFrame(frameData)) {
        return false;
    }

    size_t dataSize = frameData.width * frameData.height * getBytesPerPixel(frameData.format);
    lastFrameBuffer_.assign(frameData.data, frameData.data + dataSize);
    lastFrameData_ = frameData;
    lastFrameData_.data = lastFrameBuffer_.data();
    hasLastFrame_ = true;

    encodedFrameCount_++;
    return true;
}

bool FFmpegWrapper::step5_encodeAndWriteFrame(const FrameData& frameData) {
    if (!convertPixelFormat(frameData, frame_.get())) {
        lastError_ = "Failed to convert pixel format";
        LOG_ERROR(lastError_);
        return false;
    }

    frame_->pts = encodedFrameCount_;

    int ret = avcodec_send_frame(codecContext_.get(), frame_.get());
    if (ret < 0) {
        lastError_ = "Error sending frame to encoder";
        LOG_ERROR(lastError_);
        return false;
    }

    while (ret >= 0) {
        ret = avcodec_receive_packet(codecContext_.get(), packet_.get());
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
            break;
        } else if (ret < 0) {
            lastError_ = "Error receiving packet";
            LOG_ERROR(lastError_);
            return false;
        }

        if (!step6_muxAndWritePacket(packet_.get(), videoStream_)) {
            lastError_ = "Failed to write packet";
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

    uint8_t* inData[1] = {const_cast<uint8_t*>(audioData.data.data())};
    int inSamples = audioData.samplesPerChannel;

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

    int frameSize = audioCodecContext_->frame_size;
    while (av_audio_fifo_size(audioFifo_.get()) >= frameSize) {
        int ret = av_frame_make_writable(audioFrame_.get());
        if (ret < 0) {
            break;
        }

        if (av_audio_fifo_read(audioFifo_.get(), (void**)audioFrame_->data, frameSize) <
            frameSize) {
            break;
        }

        audioFrame_->pts = audioSamplesCount_;
        audioSamplesCount_ += frameSize;

        ret = avcodec_send_frame(audioCodecContext_.get(), audioFrame_.get());
        if (ret < 0) {
            break;
        }

        while (ret >= 0) {
            ret = avcodec_receive_packet(audioCodecContext_.get(), audioPacket_.get());
            if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
                break;
            }
            if (ret < 0) {
                return false;
            }

            if (!step6_muxAndWritePacket(audioPacket_.get(), audioStream_)) {
                return false;
            }
            av_packet_unref(audioPacket_.get());
        }
    }

    return true;
}

bool FFmpegWrapper::step6_flushEncoder() {
    std::lock_guard<std::mutex> lock(muxerMutex_);
    LOG_INFO("Flushing encoder...");

    int flushedVideoFrames = 0;
    int flushedAudioFrames = 0;

    if (codecContext_) {
        int ret = avcodec_send_frame(codecContext_.get(), nullptr);
        if (ret < 0 && ret != AVERROR_EOF) {
            LOG_WARN("avcodec_send_frame(nullptr) returned: " + std::to_string(ret));
        }

        while (true) {
            ret = avcodec_receive_packet(codecContext_.get(), packet_.get());
            if (ret == AVERROR(EAGAIN)) {
                LOG_WARN("Unexpected EAGAIN during flush");
                break;
            } else if (ret == AVERROR_EOF) {
                break;
            } else if (ret < 0) {
                LOG_ERROR("Error during video encoder flush: " + std::to_string(ret));
                break;
            }

            packet_->stream_index = videoStream_->index;
            av_packet_rescale_ts(packet_.get(), codecContext_->time_base, videoStream_->time_base);

            int writeRet = av_interleaved_write_frame(formatContext_.get(), packet_.get());
            if (writeRet < 0) {
                LOG_ERROR("Error writing flushed video packet: " + std::to_string(writeRet));
            } else {
                flushedVideoFrames++;
            }
            av_packet_unref(packet_.get());
        }

        LOG_INFO("Flushed " + std::to_string(flushedVideoFrames) + " video frames");
    }

    if (audioCodecContext_) {
        int ret = avcodec_send_frame(audioCodecContext_.get(), nullptr);
        if (ret < 0 && ret != AVERROR_EOF) {
            LOG_WARN("Audio avcodec_send_frame(nullptr) returned: " + std::to_string(ret));
        }

        while (true) {
            ret = avcodec_receive_packet(audioCodecContext_.get(), audioPacket_.get());
            if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
                break;
            } else if (ret < 0) {
                LOG_ERROR("Error during audio encoder flush: " + std::to_string(ret));
                break;
            }

            audioPacket_->stream_index = audioStream_->index;
            av_packet_rescale_ts(audioPacket_.get(), audioCodecContext_->time_base,
                                 audioStream_->time_base);

            int writeRet = av_interleaved_write_frame(formatContext_.get(), audioPacket_.get());
            if (writeRet < 0) {
                LOG_ERROR("Error writing flushed audio packet: " + std::to_string(writeRet));
            } else {
                flushedAudioFrames++;
            }
            av_packet_unref(audioPacket_.get());
        }

        if (flushedAudioFrames > 0) {
            LOG_INFO("Flushed " + std::to_string(flushedAudioFrames) + " audio frames");
        }
    }

    LOG_INFO("Encoder flushed");
    return true;
}

bool FFmpegWrapper::step6_muxAndWritePacket(AVPacket* packet, AVStream* stream) {
    std::lock_guard<std::mutex> lock(muxerMutex_);
    packet->stream_index = stream->index;

    AVCodecContext* ctx = (stream == videoStream_) ? codecContext_.get() : audioCodecContext_.get();
    av_packet_rescale_ts(packet, ctx->time_base, stream->time_base);

    int ret = av_interleaved_write_frame(formatContext_.get(), packet);
    if (ret < 0) {
        LOG_ERROR("Error writing packet to output file");
        return false;
    }
    return true;
}

void FFmpegWrapper::step7_writeTrailerAndCleanup() {
    LOG_INFO("Writing trailer and cleaning up...");

    if (formatContext_) {
        av_write_trailer(formatContext_.get());
    }

    if (formatContext_ && !(formatContext_->oformat->flags & AVFMT_NOFILE)) {
        avio_closep(&formatContext_->pb);
    }

    cleanup();

    LOG_INFO("Cleanup finished");
}

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

    int bytesPerPixel = getBytesPerPixel(frameData.format);
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

    if (hasLastFrame_) {
        LOG_INFO("Duplicating last frame to ensure proper duration...");
        step5_encodeAndWriteFrame(lastFrameData_);
        encodedFrameCount_++;
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
