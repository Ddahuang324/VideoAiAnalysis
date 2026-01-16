#include <atomic>
#include <chrono>
#include <csignal>
#include <memory>
#include <string>
#include <thread>

#include "CommandProtocol.h"
#include "IPCServer.h"
#include "Log.h"
#include "RecorderAPI.h"
#include "nlohmann/json_fwd.hpp"

using namespace Recorder;
using namespace IPC;

static std::atomic<bool> g_shouldExit{false};
static std::unique_ptr<RecorderAPI> g_recorderApi = nullptr;
static std::unique_ptr<IPCServer> g_ipcServer = nullptr;

void signalHandler(int signal) {
    LOG_INFO("Received signal: " + std::to_string(signal));
    g_shouldExit = true;
}

// 使用统一配置系统加载配置
RecorderConfig loadConfig(const std::string& configPath) {
    RecorderConfig config;

    if (configPath.empty()) {
        LOG_INFO("No config file path provided, using default configuration.");
        return config;
    }

    if (!config.loadFromFile(configPath)) {
        LOG_ERROR("Failed to load config from " + configPath + ", using defaults.");
    } else {
        LOG_INFO("Configuration loaded from " + configPath);
    }

    return config;
}

void registerIPCHandlers(IPCServer& server, RecorderAPI& recorderApi) {
    // PING 命令处理器
    server.registerHandler(CommandType::PING,
                           [&](const CommandRequest& request) -> CommandResponse {
                               return {ResponseCode::SUCCESS, "Pong", nlohmann::json{}};
                           });

    // GET_STATUS 命令处理器
    server.registerHandler(CommandType::GET_STATUS,
                           [&](const CommandRequest& request) -> CommandResponse {
                               RecordingStatus status = recorderApi.getStatus();
                               nlohmann::json data = {{"status", static_cast<int>(status)}};
                               return {ResponseCode::SUCCESS, "Status retrieved", data};
                           });

    // GET_STATS 命令处理器
    server.registerHandler(CommandType::GET_STATS,
                           [&](const CommandRequest& request) -> CommandResponse {
                               RecordingStats stats = recorderApi.getStats();
                               nlohmann::json data = {{"frame_count", stats.frame_count},
                                                      {"encoded_count", stats.encoded_count},
                                                      {"dropped_count", stats.dropped_count},
                                                      {"file_size_bytes", stats.file_size_bytes},
                                                      {"current_fps", stats.current_fps},
                                                      {"duration_seconds", stats.duration_seconds}};
                               return {ResponseCode::SUCCESS, "Stats retrieved", data};
                           });

    // START_RECORDING 命令处理器
    server.registerHandler(
        CommandType::START_RECORDING, [&](const CommandRequest& request) -> CommandResponse {
            if (recorderApi.start()) {
                return {ResponseCode::SUCCESS, "Recording started", nlohmann::json{}};
            } else {
                return {ResponseCode::ERROR_INTERNAL, "Failed to start recording",
                        nlohmann::json{}};
            }
        });

    // STOP_RECORDING 命令处理器
    server.registerHandler(
        CommandType::STOP_RECORDING, [&](const CommandRequest& request) -> CommandResponse {
            if (recorderApi.stop()) {
                return {ResponseCode::SUCCESS, "Recording stopped", nlohmann::json{}};
            } else {
                return {ResponseCode::ERROR_INTERNAL, "Failed to stop recording", nlohmann::json{}};
            }
        });

    // PAUSE_RECORDING 命令处理器
    server.registerHandler(
        CommandType::PAUSE_RECORDING, [&](const CommandRequest& request) -> CommandResponse {
            if (recorderApi.pause()) {
                return {ResponseCode::SUCCESS, "Recording paused", nlohmann::json{}};
            } else {
                return {ResponseCode::ERROR_INTERNAL, "Failed to pause recording",
                        nlohmann::json{}};
            }
        });

    // RESUME_RECORDING 命令处理器
    server.registerHandler(
        CommandType::RESUME_RECORDING, [&](const CommandRequest& request) -> CommandResponse {
            if (recorderApi.resume()) {
                return {ResponseCode::SUCCESS, "Recording resumed", nlohmann::json{}};
            } else {
                return {ResponseCode::ERROR_INTERNAL, "Failed to resume recording",
                        nlohmann::json{}};
            }
        });

    // SHUTDOWN 命令处理器
    server.registerHandler(
        CommandType::SHUTDOWN, [&](const CommandRequest& request) -> CommandResponse {
            recorderApi.shutdown();
            g_shouldExit = true;
            return {ResponseCode::SUCCESS, "Shutdown initiated", nlohmann::json{}};
        });
}

int main(int argc, char* argv[]) {
    // 设置日志
    Infra::Logger::getInstance().setLogFile("RecorderProcess.log");
    Infra::Logger::getInstance().setLogLevel(Infra::Level::INFO);

    LOG_INFO("RecorderProcess starting...");

    // 信号处理
    std::signal(SIGINT, signalHandler);
    std::signal(SIGTERM, signalHandler);

    // 解析命令行参数
    std::string configPath;
    std::string controlEndpoint = "tcp://*:7777";

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--config" && i + 1 < argc) {
            configPath = argv[++i];
        } else if (arg == "--control-port" && i + 1 < argc) {
            controlEndpoint = std::string("tcp://*:") + argv[++i];
        }
    }

    // 加载配置
    RecorderConfig config = loadConfig(configPath);

    // 初始化 RecorderAPI
    g_recorderApi = std::make_unique<RecorderAPI>();
    if (!g_recorderApi->initialize(config)) {
        LOG_ERROR("Failed to initialize RecorderAPI: " + g_recorderApi->getLastError());
        return -1;
    }

    // 初始化 IPC Server
    g_ipcServer = std::make_unique<IPCServer>(controlEndpoint);
    registerIPCHandlers(*g_ipcServer, *g_recorderApi);

    if (!g_ipcServer->start()) {
        LOG_ERROR("Failed to start IPC Server on " + controlEndpoint);
        return -1;
    }

    LOG_INFO("RecorderProcess is running on " + controlEndpoint);

    // 主循环
    while (!g_shouldExit) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    LOG_INFO("RecorderProcess shutting down...");

    // 清理资源
    g_ipcServer->stop();
    g_recorderApi->shutdown();

    LOG_INFO("RecorderProcess exited.");
    return 0;
}
