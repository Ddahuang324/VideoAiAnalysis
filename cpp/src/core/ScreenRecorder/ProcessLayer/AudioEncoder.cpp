#include "AudioEncoder.h"

#include <chrono>
#include <memory>
#include <thread>
#include <utility>

#include "AudioData.h"
#include "FFmpegWrapper.h"
#include "Log.h"
#include "ThreadSafetyQueue.h"

AudioEncoder::AudioEncoder(std::shared_ptr<ThreadSafetyQueue<AudioData>> queue,
                           std::shared_ptr<FFmpegWrapper> encoder)
    : queue_(std::move(queue)), encoder_(std::move(encoder)) {
    LOG_INFO("AudioEncoder constructed");
}

AudioEncoder::~AudioEncoder() {
    stop();
    LOG_INFO("AudioEncoder destructed");
}

void AudioEncoder::start() {
    if (isRunning_.load()) {
        return;
    }

    isRunning_.store(true);
    thread_ = std::make_unique<std::thread>(&AudioEncoder::encodeLoop, this);
    LOG_INFO("AudioEncoder thread started");
}

void AudioEncoder::stop() {
    if (!isRunning_.load()) {
        return;
    }

    isRunning_.store(false);

    if (queue_) {
        queue_->stop();
    }

    if (thread_ && thread_->joinable()) {
        thread_->join();
        thread_.reset();
    }

    LOG_INFO("AudioEncoder thread stopped");
}

void AudioEncoder::encodeLoop() {
    LOG_INFO("AudioEncoder loop started");

    constexpr auto POP_TIMEOUT = std::chrono::milliseconds(1000);

    while (isRunning_.load()) {
        AudioData audioData;

        if (!queue_->pop(audioData, POP_TIMEOUT)) {
            if (!isRunning_.load()) {
                break;
            }
            continue;
        }

        if (encoder_ && !encoder_->encodeAudioFrame(audioData)) {
            LOG_ERROR("Failed to encode audio frame");
        }
    }

    LOG_INFO("AudioEncoder loop finished");
}
