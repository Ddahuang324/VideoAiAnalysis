#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <cstdint>
#include <string>
#include <thread>

#include "Process/Analyzer/AnalyzerAPI.h"
#include "Process/Recorder/RecorderAPI.h"

using namespace Analyzer;
using namespace Recorder;

TEST(AnalyzerAPI_Basic, InterfaceCompletion) {
    AnalyzerAPI api;

    // 初始状态应为 IDLE
    EXPECT_EQ(api.getStatus(), AnalysisStatus::IDLE);

    // stats 默认值
    auto stats = api.getStats();
    EXPECT_EQ(stats.analyzedFrameCount, 0);

    // 未初始化时没有错误信息
    EXPECT_TRUE(api.getLastError().empty());

    // 尝试初始化：允许两种结果（成功或失败），但行为需自洽
    AnalyzerConfig cfg;
    bool inited = api.initialize(cfg);
    if (inited) {
        EXPECT_EQ(api.getStatus(), AnalysisStatus::IDLE);
        auto s2 = api.getStats();
        EXPECT_GE(s2.analyzedFrameCount, 0);
        // 设置回调不应崩溃
        api.setStatusCallback([](AnalysisStatus) {});
        api.setKeyFrameCallback([](int64_t) {});
        api.shutdown();
    } else {
        // 初始化失败时应有错误描述或处于 ERROR 状态
        EXPECT_FALSE(api.getLastError().empty());
        auto st = api.getStatus();
        EXPECT_TRUE(st == AnalysisStatus::ERROR || st == AnalysisStatus::IDLE);
    }
}

TEST(AnalyzerAPI_Lifecycle, FullFlow) {
    AnalyzerAPI api;
    AnalyzerConfig cfg;
    // 设置基础模型路径，这通常是初始化成功的关键
    cfg.models.basePath = "./Models";

    // 如果初始化失败（例如缺少模型文件），测试仍然自洽
    if (api.initialize(cfg)) {
        EXPECT_EQ(api.getStatus(), AnalysisStatus::IDLE);

        // 状态回调测试
        std::atomic<bool> callbackCalled{false};
        api.setStatusCallback([&](AnalysisStatus s) {
            if (s == AnalysisStatus::RUNNING)
                callbackCalled = true;
        });

        // 启动测试
        EXPECT_TRUE(api.start());
        // 某些 API 可能是异步启动，这里等待一下
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        EXPECT_EQ(api.getStatus(), AnalysisStatus::RUNNING);
        EXPECT_TRUE(callbackCalled);

        // 停止测试
        EXPECT_TRUE(api.stop());
        EXPECT_EQ(api.getStatus(), AnalysisStatus::IDLE);

        api.shutdown();
    }
}

TEST(RecorderAPI_Basic, InterfaceCompletion) {
    RecorderAPI r;

    // 初始状态应为 IDLE
    EXPECT_EQ(r.getStatus(), RecordingStatus::IDLE);

    // stats 应为零初始化
    auto stats = r.getStats();
    EXPECT_EQ(stats.frame_count, 0);
    EXPECT_EQ(stats.encoded_count, 0);
    EXPECT_EQ(stats.dropped_count, 0);

    // 初始化应返回 true（当前实现简单返回 true）
    RecorderConfig cfg;
    bool ok = r.initialize(cfg);
    EXPECT_TRUE(ok);

    // 在未开始录制时 pause/resume 返回 false
    EXPECT_FALSE(r.pause());
    EXPECT_FALSE(r.resume());

    // 设置回调
    r.setStatusCallback([](RecordingStatus) {});
    r.setErrorCallback([](const std::string&) {});

    // stop 与 shutdown 在未开始时也应安全返回
    EXPECT_TRUE(r.stop());
    r.shutdown();
}

TEST(RecorderAPI_Lifecycle, FullFlow) {
    RecorderAPI r;
    RecorderConfig cfg;
    cfg.video.outputFilePath = "test_output.mp4";

    ASSERT_TRUE(r.initialize(cfg));

    // 验证状态切换：Idle -> Recording -> Paused -> Recording -> Idle
    EXPECT_TRUE(r.start());
    EXPECT_EQ(r.getStatus(), RecordingStatus::RECORDING);

    EXPECT_TRUE(r.pause());
    EXPECT_EQ(r.getStatus(), RecordingStatus::PAUSED);

    EXPECT_TRUE(r.resume());
    EXPECT_EQ(r.getStatus(), RecordingStatus::RECORDING);

    // 检查统计信息是否可获取
    auto stats = r.getStats();
    EXPECT_GE(stats.duration_seconds, 0.0);

    EXPECT_TRUE(r.stop());
    // 有些实现 stop 后直接回 IDLE，有些可能经过 STOPPING
    auto finalStatus = r.getStatus();
    EXPECT_TRUE(finalStatus == RecordingStatus::IDLE || finalStatus == RecordingStatus::STOPPING);

    r.shutdown();
}
