#pragma once

#include <cstdint>
#include <functional>
#include <memory>
#include <string>

namespace Recorder {

enum class RecordingStatus { IDLE, INITIALIZING, RECORDING, PAUSED, STOPPING, ERROR };

struct RecordingStats {
    int64_t frame_count;
    int64_t encoded_count;
    int64_t dropped_count;
    int64_t file_size_bytes;
    double current_fps;
    double duration_seconds;
};

struct RecorderConfig {
    std::string output_file_path;
    int width;
    int height;
    bool enable_audio;
    int audio_sample_rate;
    int audio_channels;
    std::string zmqPublisher_endpoint = "tcp://*:5555";
};

class RecorderAPI {
public:
    RecorderAPI();
    ~RecorderAPI();

    bool initialize(const RecorderConfig& config);
    bool start();
    bool pause();
    bool resume();
    bool stop();
    void shutdown();

    RecordingStatus getStatus() const;
    RecordingStats getStats() const;
    std::string getLastError() const;

    using StatusCallBack = std::function<void(RecordingStatus)>;
    using ErrorCallBack = std::function<void(const std::string&)>;

    void setStatusCallback(StatusCallBack callback);
    void setErrorCallback(ErrorCallBack callback);

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

}  // namespace Recorder