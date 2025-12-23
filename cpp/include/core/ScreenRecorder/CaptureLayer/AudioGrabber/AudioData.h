#pragma once

#include <cstdint>
#include <vector>

/**
 * @brief 原始音频数据结构
 * 用于在采集层 (AudioGrabber) 和处理层 (AudioEncoder) 之间传输 PCM 数据
 */
struct AudioData {
    std::vector<uint8_t> data;  // PCM 原始数据
    int64_t timestamp_ms;       // 采集时间戳 (毫秒)
    int sampleRate;             // 采样率 (如 48000)
    int channels;               // 通道数 (如 2)
    int samplesPerChannel;      // 每个通道的样本数 (如 1024)
    int bitsPerSample;          // 位深 (如 16)

    AudioData()
        : timestamp_ms(0),
          sampleRate(48000),
          channels(2),
          samplesPerChannel(0),
          bitsPerSample(16) {}
};
