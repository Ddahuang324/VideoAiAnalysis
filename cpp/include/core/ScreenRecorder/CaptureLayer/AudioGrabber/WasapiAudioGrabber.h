#pragma once

#include <audioclient.h>
#include <mmdeviceapi.h>

#include <atomic>
#include <functional>
#include <thread>

#include "AudioData.h"
#include "AudioGrabber.h"
#include "mmeapi.h"

/**
 * @brief 基于 Windows WASAPI 的音频采集器
 * 实现系统回放声音 (Loopback) 的采集
 */
class WasapiAudioGrabber : public AudioGrabber {
public:
    WasapiAudioGrabber();
    ~WasapiAudioGrabber() override;

    bool start() override;
    void stop() override;
    void setCallback(std::function<void(const AudioData&)> callback) override;

    int getSampleRate() const override { return sampleRate_; }
    int getChannels() const override { return channels_; }

private:
    void captureThreadProc();
    bool initializeWasapi();
    void cleanupWasapi();

    std::function<void(const AudioData&)> callback_;

    std::thread captureThread_;
    std::atomic<bool> isRunning_{false};

    // WASAPI 相关接口
    IMMDeviceEnumerator* enumerator_ = nullptr;
    IMMDevice* device_ = nullptr;
    IAudioClient* audioClient_ = nullptr;
    IAudioCaptureClient* captureClient_ = nullptr;
    WAVEFORMATEX* waveFormat_ = nullptr;

    int sampleRate_ = 48000;
    int channels_ = 2;
    int bitsPerSample_ = 16;
};
