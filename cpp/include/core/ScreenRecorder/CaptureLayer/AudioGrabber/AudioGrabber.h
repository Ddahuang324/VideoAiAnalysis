#pragma once

#include <functional>

#include "AudioData.h"

/**
 * @brief 音频采集接口类
 * 定义了音频采集器的基本行为，支持回调模式获取 PCM 数据
 */
class AudioGrabber {
public:
    virtual ~AudioGrabber() = default;

    /**
     * @brief 开始音频采集
     * @return true 如果启动成功，false 否则
     */
    virtual bool start() = 0;

    /**
     * @brief 停止音频采集
     */
    virtual void stop() = 0;

    /**
     * @brief 设置音频数据回调函数
     * @param callback 当采集到新的音频数据时调用的函数
     */
    virtual void setCallback(std::function<void(const AudioData&)> callback) = 0;

    /**
     * @brief 获取当前采样率
     */
    virtual int getSampleRate() const = 0;

    /**
     * @brief 获取当前通道数
     */
    virtual int getChannels() const = 0;
};
