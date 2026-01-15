#include <cstdint>
#include <functional>
#include <memory>
#include <string>

#include "core/Config/UnifiedConfig.h"
#include "core/ScreenRecorder/ScreenRecorder.h"  // For RecorderMode

namespace Recorder {

// 导出 RecorderMode 枚举供外部使用
using RecorderMode = ::RecorderMode;

enum class RecordingStatus { IDLE, INITIALIZING, RECORDING, PAUSED, STOPPING, ERROR };

struct RecordingStats {
    int64_t frame_count;
    int64_t encoded_count;
    int64_t dropped_count;
    int64_t file_size_bytes;
    double current_fps;
    double duration_seconds;
};

// 使用统一配置系统的类型别名
using RecorderConfig = Config::RecorderConfig;

class RecorderAPI {
public:
    RecorderAPI();
    ~RecorderAPI();

    bool initialize(const RecorderConfig& config);
    bool start();
    bool pause();
    bool resume();
    bool stop();
    bool gracefulStop(int timeoutMs = 5000);  // 优雅停止,等待AI分析完成
    void shutdown();

    RecordingStatus getStatus() const;
    RecordingStats getStats() const;
    std::string getLastError() const;

    using StatusCallBack = std::function<void(RecordingStatus)>;
    using ErrorCallBack = std::function<void(const std::string&)>;

    void setStatusCallback(StatusCallBack callback);
    void setErrorCallback(ErrorCallBack callback);

    // 设置录制模式 (VIDEO 或 SNAPSHOT)
    void setRecordingMode(RecorderMode mode);
    RecorderMode getRecordingMode() const;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

}  // namespace Recorder