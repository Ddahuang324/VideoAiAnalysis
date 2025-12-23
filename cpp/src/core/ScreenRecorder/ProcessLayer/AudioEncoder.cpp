#include "AudioEncoder.h"

#include <chrono>
#include <memory>
#include <thread>

#include "AudioData.h"
#include "FFmpegWrapper.h"
#include "Log.h"
#include "ThreadSafetyQueue.h"

AudioEncoder::AudioEncoder(std::shared_ptr<ThreadSafetyQueue<AudioData>> queue,
                           std::shared_ptr<FFmpegWrapper> encoder)
    : queue_(queue), encoder_(encoder) {
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
    thread_ = std::make_unique<std::thread>([this]() { encodeLoop(); });
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

    while (isRunning_.load()) {
        AudioData audioData;

        if (!queue_->pop(audioData, std::chrono::milliseconds(1000))) {
            if (!isRunning_.load()) {
                break;
            }
            continue;
        }

        if (encoder_) {
            if (!encoder_->encodeAudioFrame(audioData)) {
                LOG_ERROR("Failed to encode audio frame");
            }
        }
    }

    LOG_INFO("AudioEncoder loop finished");
}
