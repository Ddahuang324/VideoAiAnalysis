#include "core/KeyFrame/KeyFrameVideoEncoder.h"

#include <cstddef>
#include <filesystem>
#include <string>
#include <vector>

#include "Infra/Log.h"
#include "opencv2/core/mat.hpp"
#include "opencv2/core/types.hpp"
#include "opencv2/videoio.hpp"

namespace KeyFrame {

KeyFrameVideoEncoder::KeyFrameVideoEncoder(const Config& config) : config_(config) {}

bool KeyFrameVideoEncoder::encodeKeyFrames(const std::string& sourceVideoPath,
                                           const std::vector<int>& keyFrameIndices,
                                           const std::string& outputPath) {
    if (keyFrameIndices.empty()) {
        LOG_WARN("[KeyFrameVideoEncoder] No keyframes to encode");
        return false;
    }

    if (!std::filesystem::exists(sourceVideoPath)) {
        LOG_ERROR("[KeyFrameVideoEncoder] Source video not found: " + sourceVideoPath);
        return false;
    }

    LOG_INFO("[KeyFrameVideoEncoder] Starting keyframe video encoding...");
    LOG_INFO("[KeyFrameVideoEncoder] Source: " + sourceVideoPath);
    LOG_INFO("[KeyFrameVideoEncoder] Output: " + outputPath);
    LOG_INFO("[KeyFrameVideoEncoder] Keyframes: " + std::to_string(keyFrameIndices.size()));

    // 打开源视频
    cv::VideoCapture capture(sourceVideoPath);
    if (!capture.isOpened()) {
        LOG_ERROR("[KeyFrameVideoEncoder] Failed to open source video");
        return false;
    }

    // 获取源视频属性
    int width = static_cast<int>(capture.get(cv::CAP_PROP_FRAME_WIDTH));
    int height = static_cast<int>(capture.get(cv::CAP_PROP_FRAME_HEIGHT));

    if (width <= 0 || height <= 0) {
        LOG_ERROR("[KeyFrameVideoEncoder] Invalid video dimensions");
        return false;
    }

    LOG_INFO("[KeyFrameVideoEncoder] Video dimensions: " + std::to_string(width) + "x" +
             std::to_string(height));

    // 创建视频写入器
    cv::VideoWriter writer;
    int fourcc = cv::VideoWriter::fourcc('m', 'p', '4', 'v');  // MP4V编码器

    if (!writer.open(outputPath, fourcc, config_.outputFPS, cv::Size(width, height), true)) {
        LOG_ERROR("[KeyFrameVideoEncoder] Failed to create video writer");
        return false;
    }

    // 提取并写入关键帧
    int successCount = 0;
    int lastFrameIndex = -1;

    for (size_t i = 0; i < keyFrameIndices.size(); ++i) {
        int frameIndex = keyFrameIndices[i];

        // 确保索引递增
        if (frameIndex <= lastFrameIndex) {
            LOG_WARN("[KeyFrameVideoEncoder] Skipping duplicate or out-of-order frame: " +
                     std::to_string(frameIndex));
            continue;
        }

        cv::Mat frame;
        if (!seekAndReadFrame(capture, frameIndex, frame)) {
            LOG_WARN("[KeyFrameVideoEncoder] Failed to read frame " + std::to_string(frameIndex));
            continue;
        }

        // 写入帧
        writer.write(frame);
        successCount++;
        lastFrameIndex = frameIndex;

        // 进度日志
        if ((i + 1) % 10 == 0 || i == keyFrameIndices.size() - 1) {
            LOG_INFO("[KeyFrameVideoEncoder] Progress: " + std::to_string(i + 1) + "/" +
                     std::to_string(keyFrameIndices.size()) + " frames encoded");
        }
    }

    // 释放资源
    writer.release();
    capture.release();

    if (successCount > 0) {
        LOG_INFO("[KeyFrameVideoEncoder] Encoding completed successfully. Total frames: " +
                 std::to_string(successCount));
        return true;
    } else {
        LOG_ERROR("[KeyFrameVideoEncoder] No frames were successfully encoded");
        return false;
    }
}

std::string KeyFrameVideoEncoder::generateOutputPath(const std::string& sourceVideoPath) {
    std::filesystem::path sourcePath(sourceVideoPath);
    std::string stem = sourcePath.stem().string();
    std::string extension = sourcePath.extension().string();
    std::filesystem::path parentPath = sourcePath.parent_path();

    std::string outputFilename = stem + "_keyframes" + extension;
    return (parentPath / outputFilename).string();
}

bool KeyFrameVideoEncoder::seekAndReadFrame(cv::VideoCapture& capture, int frameIndex,
                                            cv::Mat& frame) {
    // 设置帧位置
    if (!capture.set(cv::CAP_PROP_POS_FRAMES, static_cast<double>(frameIndex))) {
        LOG_ERROR("[KeyFrameVideoEncoder] Failed to seek to frame " + std::to_string(frameIndex));
        return false;
    }

    // 读取帧
    if (!capture.read(frame)) {
        LOG_ERROR("[KeyFrameVideoEncoder] Failed to read frame " + std::to_string(frameIndex));
        return false;
    }

    if (frame.empty()) {
        LOG_ERROR("[KeyFrameVideoEncoder] Read empty frame at index " + std::to_string(frameIndex));
        return false;
    }

    return true;
}

bool KeyFrameVideoEncoder::mergeAudioFromSource(const std::string& keyframeVideoPath,
                                                 const std::string& sourceVideoPath,
                                                 const std::string& outputPath) {
    if (!std::filesystem::exists(keyframeVideoPath) || !std::filesystem::exists(sourceVideoPath)) {
        LOG_ERROR("[KeyFrameVideoEncoder] Input file not found");
        return false;
    }

    AVFormatContext* kfFmtCtx = nullptr;
    AVFormatContext* srcFmtCtx = nullptr;
    AVFormatContext* outFmtCtx = nullptr;

    // 打开关键帧视频
    if (avformat_open_input(&kfFmtCtx, keyframeVideoPath.c_str(), nullptr, nullptr) < 0) {
        LOG_ERROR("[KeyFrameVideoEncoder] Failed to open keyframe video");
        return false;
    }
    avformat_find_stream_info(kfFmtCtx, nullptr);

    // 打开源视频（取音频）
    if (avformat_open_input(&srcFmtCtx, sourceVideoPath.c_str(), nullptr, nullptr) < 0) {
        avformat_close_input(&kfFmtCtx);
        LOG_ERROR("[KeyFrameVideoEncoder] Failed to open source video");
        return false;
    }
    avformat_find_stream_info(srcFmtCtx, nullptr);

    // 找到视频流和音频流索引
    int kfVideoIdx = -1, srcAudioIdx = -1;
    for (unsigned i = 0; i < kfFmtCtx->nb_streams; i++) {
        if (kfFmtCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            kfVideoIdx = i;
            break;
        }
    }
    for (unsigned i = 0; i < srcFmtCtx->nb_streams; i++) {
        if (srcFmtCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
            srcAudioIdx = i;
            break;
        }
    }

    if (kfVideoIdx < 0 || srcAudioIdx < 0) {
        LOG_ERROR("[KeyFrameVideoEncoder] Video or audio stream not found");
        avformat_close_input(&kfFmtCtx);
        avformat_close_input(&srcFmtCtx);
        return false;
    }

    // 计算拉伸比例
    double kfDuration = kfFmtCtx->duration / (double)AV_TIME_BASE;
    double srcDuration = srcFmtCtx->duration / (double)AV_TIME_BASE;
    double stretchFactor = srcDuration / kfDuration;
    LOG_INFO("[KeyFrameVideoEncoder] Stretch: " + std::to_string(stretchFactor));

    // 创建输出
    avformat_alloc_output_context2(&outFmtCtx, nullptr, nullptr, outputPath.c_str());
    if (!outFmtCtx) {
        avformat_close_input(&kfFmtCtx);
        avformat_close_input(&srcFmtCtx);
        return false;
    }

    // 添加视频流
    AVStream* outVideoStream = avformat_new_stream(outFmtCtx, nullptr);
    avcodec_parameters_copy(outVideoStream->codecpar, kfFmtCtx->streams[kfVideoIdx]->codecpar);
    outVideoStream->codecpar->codec_tag = 0;
    // 调整时间基以匹配拉伸后的时长
    outVideoStream->time_base = kfFmtCtx->streams[kfVideoIdx]->time_base;

    // 添加音频流
    AVStream* outAudioStream = avformat_new_stream(outFmtCtx, nullptr);
    avcodec_parameters_copy(outAudioStream->codecpar, srcFmtCtx->streams[srcAudioIdx]->codecpar);
    outAudioStream->codecpar->codec_tag = 0;
    outAudioStream->time_base = srcFmtCtx->streams[srcAudioIdx]->time_base;

    // 打开输出文件
    if (!(outFmtCtx->oformat->flags & AVFMT_NOFILE)) {
        if (avio_open(&outFmtCtx->pb, outputPath.c_str(), AVIO_FLAG_WRITE) < 0) {
            avformat_close_input(&kfFmtCtx);
            avformat_close_input(&srcFmtCtx);
            avformat_free_context(outFmtCtx);
            return false;
        }
    }

    avformat_write_header(outFmtCtx, nullptr);

    AVPacket pkt;
    // 写入视频包（拉伸时间戳）
    while (av_read_frame(kfFmtCtx, &pkt) >= 0) {
        if (pkt.stream_index == kfVideoIdx) {
            pkt.stream_index = 0;
            // 拉伸 PTS/DTS
            pkt.pts = (int64_t)(pkt.pts * stretchFactor);
            pkt.dts = (int64_t)(pkt.dts * stretchFactor);
            pkt.duration = (int64_t)(pkt.duration * stretchFactor);
            av_interleaved_write_frame(outFmtCtx, &pkt);
        }
        av_packet_unref(&pkt);
    }

    // 写入音频包（直接复制）
    while (av_read_frame(srcFmtCtx, &pkt) >= 0) {
        if (pkt.stream_index == srcAudioIdx) {
            pkt.stream_index = 1;
            av_packet_rescale_ts(&pkt, srcFmtCtx->streams[srcAudioIdx]->time_base,
                                 outAudioStream->time_base);
            av_interleaved_write_frame(outFmtCtx, &pkt);
        }
        av_packet_unref(&pkt);
    }

    av_write_trailer(outFmtCtx);

    // 清理
    avformat_close_input(&kfFmtCtx);
    avformat_close_input(&srcFmtCtx);
    if (!(outFmtCtx->oformat->flags & AVFMT_NOFILE)) {
        avio_closep(&outFmtCtx->pb);
    }
    avformat_free_context(outFmtCtx);

    LOG_INFO("[KeyFrameVideoEncoder] Audio merge completed: " + outputPath);
    return true;
}

}  // namespace KeyFrame
