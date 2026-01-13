#include <gtest/gtest.h>

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <opencv2/core/mat.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/opencv.hpp>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

#include "DataConverter.h"
#include "FFmpegWrapper.h"
#include "FramePublisher.h"
#include "KeyFrameMetaDataSubscriber.h"
#include "Log.h"
#include "Protocol.h"
#include "RingFrameBuffer.h"
#include "TestPathUtils.h"
#include "VideoGrabber.h"
#include "core/KeyFrame/KeyFrameAnalyzerService.h"


namespace fs = std::filesystem;

class KeyFrameMp4GeneratorTest : public ::testing::Test {
protected:
    void SetUp() override {
        // 清理输出文件
        fs::remove("simulated_keyframes.mp4");
    }

    void TearDown() override {
        // 保持输出文件供用户检视，或者在 CI 中清理
    }
};

/**
 * @brief 完整流程测试：采集端(Capture) -> KeyFrameAnalyzerService -> 采集端(Capture) -> 生成MP4
 * 修复:
 * 1. BGR到RGB颜色转换,解决色差问题
 * 2. 集成真实的KeyFrameAnalyzerService,实现关键帧过滤
 */
TEST_F(KeyFrameMp4GeneratorTest, FullFlowSimulation) {
    // 使用 TestPathUtils 查找测试资源目录
    fs::path testAssetsDir = TestPathUtils::findAssetsDir("1-anytype.png");
    ASSERT_FALSE(testAssetsDir.empty()) << "Test assets directory not found!";

    // 使用 TestPathUtils 查找模型目录
    fs::path sceneModelPath = TestPathUtils::findModelFile("MobileNet-v3-Small.onnx");
    fs::path motionModelPath = TestPathUtils::findModelFile("yolov8n.onnx");
    fs::path textDetModelPath = TestPathUtils::findModelFile("ch_PP-OCRv4_det_infer.onnx");

    ASSERT_FALSE(sceneModelPath.empty()) << "Scene model not found!";
    ASSERT_FALSE(motionModelPath.empty()) << "Motion model not found!";
    ASSERT_FALSE(textDetModelPath.empty()) << "Text detection model not found!";

    std::vector<std::string> imageFiles = {
        "1-anytype.png", "2-code.png", "3-codeWithSmallChange.png", "4.png", "5.png", "6.png"};

    // 1. 初始化 KeyFrameAnalyzerService (替换模拟的AI端)
    KeyFrame::KeyFrameAnalyzerService::Config serviceConfig;
    serviceConfig.zmqSubscriber.endpoint = "tcp://localhost:5555";
    serviceConfig.zmqPublisher.endpoint = "tcp://*:5556";

    // 模型路径 - 使用 u8string() 确保 UTF-8 编码
    serviceConfig.models.sceneModelPath = sceneModelPath.u8string();
    serviceConfig.models.motionModelPath = motionModelPath.u8string();
    serviceConfig.models.textDetModelPath = textDetModelPath.u8string();

    // 管道配置
    serviceConfig.pipeline.frameBufferSize = 10;
    serviceConfig.pipeline.scoreBufferSize = 10;
    serviceConfig.pipeline.analysisThreadCount = 2;

    // 选择策略：使用阈值模式以过滤重复/低分帧
    serviceConfig.keyframeDetector.useThresholdMode = true;
    serviceConfig.keyframeDetector.highQualityThreshold = 0.6f;
    serviceConfig.keyframeDetector.minScoreThreshold = 0.3f;

    auto analyzerService = std::make_unique<KeyFrame::KeyFrameAnalyzerService>(serviceConfig);
    ASSERT_TRUE(analyzerService->start());

    // 2. 初始化采集端组件
    MQInfra::FramePublisher capturePublisher;
    ASSERT_TRUE(capturePublisher.initialize("tcp://*:5555"));

    MQInfra::KeyFrameMetaDataSubscriber captureMetaSubscriber;
    ASSERT_TRUE(captureMetaSubscriber.initialize("tcp://localhost:5556"));

    RingFrameBuffer ringBuffer(100);

    // 初始化 FFmpeg 编码器 (用于生成关键帧 MP4)
    auto ffmpegEncoder = std::make_shared<FFmpegWrapper>();
    EncoderConfig config;
    config.video.outputFilePath = "simulated_keyframes.mp4";
    config.video.width = 1920;   // 假设宽度
    config.video.height = 1080;  // 假设高度
    config.video.fps = 10;       // 关键帧视频 fps 设置低一些也没关系
    config.video.bitrate = 4000000;
    config.video.crf = 23;
    config.video.preset = "fast";
    config.video.codec = "libx264";
    config.audio.enabled = false;

    // 等待 ZMQ 连接建立
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    LOG_INFO("Starting simulation loop...");

    bool encoderInitialized = false;

    for (size_t i = 0; i < imageFiles.size(); ++i) {
        fs::path fullPath = testAssetsDir / imageFiles[i];
        cv::Mat frame = KeyFrame::DataConverter::readImage(fullPath.u8string());
        if (frame.empty()) {
            LOG_ERROR("Failed to load image: " + fullPath.u8string());
            continue;
        }

        // 修复BGR格式问题: OpenCV读取为BGR,需要转换为RGB
        cv::cvtColor(frame, frame, cv::COLOR_BGR2RGB);

        uint64_t frameId = i + 1;
        uint64_t timestamp = i * 100;  // 每100ms一帧

        // --- 采集端动作 ---
        // A. 存入环形缓冲区
        ringBuffer.push(frameId, frame, timestamp);

        // B. 发布帧到 AI
        Protocol::FrameHeader header;
        header.magic_num = Protocol::FRAME_MAGIC;
        header.frameID = frameId;
        header.timestamp = timestamp;
        header.width = frame.cols;
        header.height = frame.rows;
        header.channels = frame.channels();
        header.data_size = frame.total() * frame.elemSize();

        // 重要：CRC 必须覆盖 Header + Data
        uint32_t computed_crc = Protocol::calculateCrc32(&header, sizeof(header));
        computed_crc = Protocol::calculateCrc32(frame.data, header.data_size, computed_crc);
        uint32_t final_crc = computed_crc ^ 0xFFFFFFFF;

        capturePublisher.publishRaw(header, frame.data, header.data_size, final_crc);
        LOG_INFO("Capture: Published frame " + std::to_string(frameId) + " -> " +
                 fullPath.u8string());

        // 给一点处理间隔,让Service有时间分析
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }

    // 3. 收集关键帧元数据 (先收集，后排序编码)
    LOG_INFO("Capture: Collecting keyframe metadata from AnalyzerService...");
    std::vector<Protocol::KeyFrameMetaDataMessage> collectedMetas;

    // 先在停止服务前尝试收集一段时间
    int maxWaitAttempts = 50;  // 增加等待时间，因为分析需要时间
    while (maxWaitAttempts-- > 0) {
        auto metaReceived = captureMetaSubscriber.receiveMetaData(200);
        if (metaReceived) {
            collectedMetas.push_back(*metaReceived);
            LOG_INFO("Capture: Collected keyframe ID " +
                     std::to_string(metaReceived->header.frameID) +
                     ", Score: " + std::to_string(metaReceived->header.Final_Score));
            if (collectedMetas.size() >= 4)
                break;
        }
    }

    // 停止服务以触发内部所有缓冲区的 Flush
    LOG_INFO("Stopping AnalyzerService to flush remaining frames...");
    analyzerService->stop();

    // 再次尝试收取可能在 Stop 时产生的 Flush 数据
    while (true) {
        auto metaReceived = captureMetaSubscriber.receiveMetaData(100);
        if (metaReceived) {
            collectedMetas.push_back(*metaReceived);
            LOG_INFO("Capture: Collected flushed keyframe ID " +
                     std::to_string(metaReceived->header.frameID) +
                     ", Score: " + std::to_string(metaReceived->header.Final_Score));
        } else {
            break;
        }
    }

    // 4. 按帧ID排序，确保视频顺序正确
    std::sort(
        collectedMetas.begin(), collectedMetas.end(),
        [](const Protocol::KeyFrameMetaDataMessage& a, const Protocol::KeyFrameMetaDataMessage& b) {
            return a.header.frameID < b.header.frameID;
        });

    // 映射 frameID -> 原始图片文件名，方便排查
    std::unordered_map<uint64_t, std::string> idToFilename;
    for (size_t i = 0; i < imageFiles.size(); ++i) {
        idToFilename[i + 1] = (testAssetsDir / imageFiles[i]).u8string();
    }

    // 打印收集到的元数据对应的真实图片名，方便定位缺失问题
    for (const auto& meta : collectedMetas) {
        uint64_t fid = meta.header.frameID;
        auto it = idToFilename.find(fid);
        if (it != idToFilename.end()) {
            LOG_INFO("Collected meta maps frame " + std::to_string(fid) + " -> " + it->second +
                     ", Score: " + std::to_string(meta.header.Final_Score));
        } else {
            LOG_INFO("Collected meta for frame " + std::to_string(fid) +
                     " -> (unknown file), Score: " + std::to_string(meta.header.Final_Score));
        }
    }

    // 5. 统一编码
    int encodedCount = 0;
    LOG_INFO("Capture: Encoding " + std::to_string(collectedMetas.size()) +
             " frames in chronological order...");

    for (const auto& meta : collectedMetas) {
        uint64_t recId = meta.header.frameID;
        uint64_t ts = 0;
        auto keyFrameMat = ringBuffer.get(recId, ts);
        if (keyFrameMat) {
            if (!encoderInitialized) {
                config.video.width = keyFrameMat->cols & ~1;
                config.video.height = keyFrameMat->rows & ~1;
                ASSERT_TRUE(ffmpegEncoder->initialize(config));
                encoderInitialized = true;
            }

            FrameData fd;
            fd.frame = *keyFrameMat;
            fd.frame_ID = recId;
            fd.timestamp_ms = ts;
            fd.data = keyFrameMat->data;
            fd.width = keyFrameMat->cols;
            fd.height = keyFrameMat->rows;
            fd.format = PixelFormat::RGB24;

            if (ffmpegEncoder->encoderFrame(fd)) {
                encodedCount++;
                LOG_INFO("Capture: Successfully encoded keyframe " + std::to_string(recId));
            } else {
                LOG_ERROR("Capture: Failed to encode keyframe " + std::to_string(recId));
            }
        }
    }

    // 6. 收尾
    if (encoderInitialized) {
        ffmpegEncoder->finalize();
    }

    capturePublisher.shutdown();
    captureMetaSubscriber.shutdown();

    LOG_INFO("Total keyframes encoded: " + std::to_string(encodedCount));

    // 验证文件是否生成
    ASSERT_TRUE(fs::exists("simulated_keyframes.mp4"));
    LOG_INFO("Test completed. Output file: simulated_keyframes.mp4 (Size: " +
             std::to_string(fs::file_size("simulated_keyframes.mp4")) + " bytes)");

    // 验证关键帧数量 (根据新阈值，预期过滤更多帧)
    EXPECT_LE(encodedCount, 4) << "Expected more frames to be filtered out with higher threshold";
    EXPECT_GE(encodedCount, 1) << "Expected at least 1 keyframe";
}
