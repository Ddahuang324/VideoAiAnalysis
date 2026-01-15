#pragma once

#include <string>
#include <vector>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/opt.h>
}

#include "opencv2/core/mat.hpp"
#include "opencv2/videoio.hpp"

namespace KeyFrame {

/**
 * @brief 关键帧视频编码器
 *
 * 负责从原始视频中提取指定索引的关键帧，并编码为独立的关键帧视频文件。
 * 支持自定义输出帧率、编码器和质量参数。
 */
class KeyFrameVideoEncoder {
public:
    struct Config {
        std::string outputCodec;   // 输出编码器
        int outputFPS;             // 关键帧视频帧率
        std::string outputPreset;  // 编码速度预设
        int outputQuality;         // CRF质量 (0-51, 越小质量越高)

        Config() : outputCodec("libx264"), outputFPS(5), outputPreset("fast"), outputQuality(23) {}
    };

    /**
     * @brief 构造函数
     * @param config 编码器配置
     */
    explicit KeyFrameVideoEncoder(const Config& config = Config());

    /**
     * @brief 从原视频中提取关键帧并编码
     *
     * @param sourceVideoPath 源视频文件路径
     * @param keyFrameIndices 关键帧索引列表（0-based，按帧号排序）
     * @param outputPath 输出视频文件路径
     * @return true 编码成功
     * @return false 编码失败
     */
    bool encodeKeyFrames(const std::string& sourceVideoPath,
                         const std::vector<int>& keyFrameIndices, const std::string& outputPath);

    /**
     * @brief 生成关键帧视频的默认输出路径
     *
     * 根据源视频路径生成对应的关键帧视频路径。
     * 例如: "recording.mp4" -> "recording_keyframes.mp4"
     *
     * @param sourceVideoPath 源视频路径
     * @return std::string 关键帧视频路径
     */
    static std::string generateOutputPath(const std::string& sourceVideoPath);

private:
    Config config_;

    /**
     * @brief 从视频中读取指定索引的帧
     *
     * @param capture 已打开的视频捕获对象
     * @param frameIndex 目标帧索引
     * @param frame 输出帧
     * @return true 读取成功
     * @return false 读取失败
     */
    bool seekAndReadFrame(cv::VideoCapture& capture, int frameIndex, cv::Mat& frame);

public:
    /**
     * @brief 将原始视频的音频合并到关键帧视频，自动拉伸视频匹配音频时长
     *
     * @param keyframeVideoPath 关键帧视频路径（无音频）
     * @param sourceVideoPath 原始视频路径（含音频）
     * @param outputPath 输出视频路径
     * @return true 合并成功
     * @return false 合并失败
     */
    static bool mergeAudioFromSource(const std::string& keyframeVideoPath,
                                     const std::string& sourceVideoPath,
                                     const std::string& outputPath);
};

}  // namespace KeyFrame
