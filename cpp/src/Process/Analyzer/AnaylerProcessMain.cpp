#include <atomic>
#include <chrono>
#include <csignal>
#include <exception>
#include <fstream>
#include <memory>
#include <string>
#include <thread>

#include "AnalyzerAPI.h"
#include "CommandProtocol.h"
#include "IPCServer.h"
#include "Infra/Log.h"

using namespace Analyzer;
using namespace IPC;

static std::atomic<bool> g_shouldExit{false};
static std::unique_ptr<AnalyzerAPI> g_analyzerAPI = nullptr;
static std::unique_ptr<IPCServer> g_ipcServer = nullptr;

void signalHandler(int signal) {
    LOG_INFO("Received signal " + std::to_string(signal) + ", shutting down...");
    g_shouldExit = true;
}

// 使用统一配置系统加载配置 (简化版)
AnalyzerConfig loadConfig(const std::string& configPath) {
    AnalyzerConfig config;

    if (configPath.empty()) {
        LOG_INFO("No config file path provided, using default configuration.");
        return config;  // 使用默认值
    }

    if (!config.loadFromFile(configPath)) {
        LOG_ERROR("Failed to load config from " + configPath + ", using defaults.");
    }

    return config;
}

void registerIPCHandlers(IPCServer& server, AnalyzerAPI& api) {
    // PING
    server.registerHandler(CommandType::PING, [](const CommandRequest& request) -> CommandResponse {
        return {ResponseCode::SUCCESS, "Pong", nlohmann::json{}};
    });

    // START_ANALYZER
    server.registerHandler(
        CommandType::START_ANALYZER, [&api](const CommandRequest& request) -> CommandResponse {
            if (api.start()) {
                return {ResponseCode::SUCCESS, "Analysis started", nlohmann::json{}};
            } else {
                return {ResponseCode::ERROR_INTERNAL, api.getLastError(), nlohmann::json{}};
            }
        });

    // STOP_ANALYZER
    server.registerHandler(
        CommandType::STOP_ANALYZER, [&api](const CommandRequest& request) -> CommandResponse {
            if (api.stop()) {
                return {ResponseCode::SUCCESS, "Analysis stopped", nlohmann::json{}};
            } else {
                return {ResponseCode::ERROR_INTERNAL, api.getLastError(), nlohmann::json{}};
            }
        });

    // GET_STATUS
    server.registerHandler(
        CommandType::GET_STATUS, [&api](const CommandRequest& request) -> CommandResponse {
            nlohmann::json data;
            data["status"] = static_cast<int>(api.getStatus());
            return {ResponseCode::SUCCESS, "Status retrieved", data};
        });

    // GET_STATS
    server.registerHandler(
        CommandType::GET_STATS, [&api](const CommandRequest& request) -> CommandResponse {
            AnalysisStats stats = api.getStats();
            nlohmann::json data;
            data["analyzed_frame_count"] = stats.analyzedFrameCount;
            data["keyframe_count"] = stats.keyframeCount;

            nlohmann::json kfList = nlohmann::json::array();
            for (const auto& kf : stats.latestKeyFrames) {
                kfList.push_back({{"frame_index", kf.frameIndex},
                                 {"score", kf.score},
                                 {"timestamp", kf.timestamp}});
            }
            data["latest_keyframes"] = kfList;
            return {ResponseCode::SUCCESS, "Stats retrieved", data};
        });

    // SHUTDOWN
    server.registerHandler(
        CommandType::SHUTDOWN, [](const CommandRequest& request) -> CommandResponse {
            g_shouldExit = true;
            return {ResponseCode::SUCCESS, "Shutdown initiated", nlohmann::json{}};
        });
}

int main(int argc, char* argv[]) {
    // 设置日志
    Infra::Logger::getInstance().setLogFile("AnalyzerProcess.log");
    Infra::Logger::getInstance().setLogLevel(Infra::Level::INFO);

    LOG_INFO("AnalyzerProcess starting...");

    // 信号处理
    std::signal(SIGINT, signalHandler);
    std::signal(SIGTERM, signalHandler);

    // 解析命令行参数
    std::string configPath;
    std::string controlEndpoint = "tcp://*:7778";
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--config" && i + 1 < argc) {
            configPath = argv[++i];
        } else if (arg == "--control-port" && i + 1 < argc) {
            controlEndpoint = std::string("tcp://*:") + argv[++i];
        }
    }

    // 初始化 API
    g_analyzerAPI = std::make_unique<AnalyzerAPI>();

    // 使用统一配置系统加载配置 (自动验证)
    AnalyzerConfig config = loadConfig(configPath);

    if (!g_analyzerAPI->initialize(config)) {
        LOG_ERROR("Failed to initialize AnalyzerAPI: " + g_analyzerAPI->getLastError());
        return -1;
    }

    // 初始化 IPC Server
    g_ipcServer = std::make_unique<IPCServer>(controlEndpoint);
    registerIPCHandlers(*g_ipcServer, *g_analyzerAPI);

    if (!g_ipcServer->start()) {
        LOG_ERROR("Failed to start IPC Server on " + controlEndpoint);
        return -1;
    }

    LOG_INFO("AnalyzerProcess is running on " + controlEndpoint);

    // 主循环
    while (!g_shouldExit) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    LOG_INFO("AnalyzerProcess shutting down...");

    g_ipcServer->stop();
    g_analyzerAPI->shutdown();

    LOG_INFO("AnalyzerProcess exited.");
    return 0;
}
