#include <gtest/gtest.h>

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <memory>
#include <opencv2/core/mat.hpp>
#include <string>
#include <thread>
#include <vector>

#ifdef _WIN32
#    ifndef NOMINMAX
#        define NOMINMAX
#    endif
#    include <windows.h>
#endif

#include "TestPathUtils.h"
#include "core/KeyFrame/Foundation/DataConverter.h"
#include "core/KeyFrame/KeyFrameAnalyzerService.h"
#include "core/MQInfra/FramePublisher.h"
#include "core/MQInfra/KeyFrameMetaDataSubscriber.h"
#include "core/MQInfra/Protocol.h"


namespace {
const std::string FRAME_ENDPOINT = "tcp://127.0.0.1:5560";
const std::string META_ENDPOINT = "tcp://127.0.0.1:5561";
}  // namespace

class ZMQKEYFRameSelectorTest : public ::testing::Test {
protected:
    void SetUp() override {
#ifdef _WIN32
        SetConsoleOutputCP(CP_UTF8);
#endif
        // 使用 TestPathUtils 查找模型路径
        auto sceneModelPath = TestPathUtils::findModelFile("MobileNet-v3-Small.onnx");
        auto motionModelPath = TestPathUtils::findModelFile("yolov8n.onnx");
        auto textDetModelPath = TestPathUtils::findModelFile("ch_PP-OCRv4_det_infer.onnx");

        ASSERT_FALSE(sceneModelPath.empty()) << "Scene model not found!";
        ASSERT_FALSE(motionModelPath.empty()) << "Motion model not found!";
        ASSERT_FALSE(textDetModelPath.empty()) << "Text detection model not found!";

        // 配置 Service
        KeyFrame::KeyFrameAnalyzerService::Config config;
        config.zmqSubscriber.endpoint = FRAME_ENDPOINT;
        config.zmqPublisher.endpoint = META_ENDPOINT;

        // 模型路径 - 使用 u8string() 确保 UTF-8 编码
        config.models.sceneModelPath = sceneModelPath.u8string();
        config.models.motionModelPath = motionModelPath.u8string();
        config.models.textDetModelPath = textDetModelPath.u8string();

        // 管道配置
        config.pipeline.frameBufferSize = 10;
        config.pipeline.scoreBufferSize = 10;
        config.pipeline.analysisThreadCount = 2;

        // 选择策略：使用阈值模式以过滤重复/低分帧
        config.keyframeDetector.useThresholdMode = true;
        config.keyframeDetector.highQualityThreshold = 0.6f;
        config.keyframeDetector.minScoreThreshold = 0.3f;

        service_ = std::make_unique<KeyFrame::KeyFrameAnalyzerService>(config);
    }

    void TearDown() override {
        if (service_) {
            service_->stop();
        }
    }

    std::unique_ptr<KeyFrame::KeyFrameAnalyzerService> service_;
};

TEST_F(ZMQKEYFRameSelectorTest, EndToEndPipelineTest) {
    // 使用 TestPathUtils 查找测试资源目录
    auto testAssetsDir = TestPathUtils::findAssetsDir("1-anytype.png");
    ASSERT_FALSE(testAssetsDir.empty()) << "Test assets directory not found!";

    // 1. 启动服务
    ASSERT_TRUE(service_->start());

    // 2. 模拟采集进程 (Publisher)
    MQInfra::FramePublisher publisher;
    ASSERT_TRUE(publisher.initialize(FRAME_ENDPOINT));

    // 3. 模拟录像进程 (Subscriber - 获取分析结果)
    MQInfra::KeyFrameMetaDataSubscriber subscriber;
    ASSERT_TRUE(subscriber.initialize(META_ENDPOINT));

    // 等待 ZMQ 连接稳定
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    // 4. 发送测试帧 (1-6)
    std::vector<std::string> imageFiles = {
        "1-anytype.png", "2-code.png", "3-codeWithSmallChange.png", "4.png", "5.png", "6.png"};

    std::cout << "[Test] Starting to send " << imageFiles.size() << " frames..." << std::endl;

    for (size_t i = 0; i < imageFiles.size(); ++i) {
        auto path = testAssetsDir / imageFiles[i];
        cv::Mat img = KeyFrame::DataConverter::readImage(path.u8string());
        ASSERT_FALSE(img.empty()) << "Failed to load: " << path;

        Protocol::FrameMessage msg;
        msg.header.frameID = static_cast<uint32_t>(i + 1);
        msg.header.width = static_cast<uint32_t>(img.cols);
        msg.header.height = static_cast<uint32_t>(img.rows);
        msg.header.channels = static_cast<uint8_t>(img.channels());
        msg.header.data_size = static_cast<uint32_t>(img.total() * img.elemSize());
        msg.image_data.assign(img.data, img.data + msg.header.data_size);

        uint32_t crc = Protocol::calculateCrc32(&msg.header, sizeof(msg.header));
        crc = Protocol::calculateCrc32(msg.image_data.data(), msg.image_data.size(), crc);
        msg.crc32 = crc ^ 0xFFFFFFFF;

        publisher.publish(msg);
        std::cout << "[Test] Published Frame ID: " << msg.header.frameID << " (" << imageFiles[i]
                  << ")" << std::endl;

        // 给一点处理间隔
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }

    // 5. 收集结果 (在停止服务前尝试收集，或在停止过程中收集)
    std::cout << "[Test] Sending completed. Waiting for results..." << std::endl;

    std::vector<uint32_t> selectedFrameIDs;
    int maxWaitAttempts = 50;  // 增加等待时间，因为分析需要时间
    while (maxWaitAttempts-- > 0) {
        auto meta = subscriber.receiveMetaData(200);
        if (meta) {
            selectedFrameIDs.push_back(meta->header.frameID);
            std::cout << "[Test] Received KeyFrame ID: " << meta->header.frameID
                      << ", Score: " << meta->header.Final_Score << std::endl;
            if (selectedFrameIDs.size() >= 4)
                break;
        }
    }

    // 6. 停止服务以触发剩余 Buffer 刷新 (如果有的话)
    std::cout << "[Test] Stopping service to flush remaining buffers..." << std::endl;
    service_->stop();

    // 再次尝试收取可能在 Stop 时产生的 Flush 数据
    while (true) {
        auto meta = subscriber.receiveMetaData(100);
        if (meta) {
            selectedFrameIDs.push_back(meta->header.frameID);
            std::cout << "[Test] Received Flushed KeyFrame ID: " << meta->header.frameID
                      << std::endl;
        } else {
            break;
        }
    }

    // 7. 验证结果
    // 理想状态：1, 2, 4, 5 应该被保留 (3 是 2 的重复/微调，6 可能与 5 相似或评分低)
    std::cout << "[Test] Selected " << selectedFrameIDs.size() << " keyframes." << std::endl;

    EXPECT_GE(selectedFrameIDs.size(), 2);  // 至少应该有核心帧

    auto it1 = std::find(selectedFrameIDs.begin(), selectedFrameIDs.end(), 1);
    auto it2 = std::find(selectedFrameIDs.begin(), selectedFrameIDs.end(), 2);
    auto it4 = std::find(selectedFrameIDs.begin(), selectedFrameIDs.end(), 4);
    auto it5 = std::find(selectedFrameIDs.begin(), selectedFrameIDs.end(), 5);

    EXPECT_NE(it1, selectedFrameIDs.end()) << "Frame 1 (AnyType) should be selected";
    EXPECT_NE(it2, selectedFrameIDs.end()) << "Frame 2 (Code) should be selected";

    // 3 和 6 不应该在高频率出现或评分应低于阈值
    auto it3 = std::find(selectedFrameIDs.begin(), selectedFrameIDs.end(), 3);
    auto it6 = std::find(selectedFrameIDs.begin(), selectedFrameIDs.end(), 6);

    // 注意：与具体的模型输出有关，如果 0.6 还是没能过滤掉，请再稍微调高
    EXPECT_EQ(it3, selectedFrameIDs.end()) << "Frame 3 (SmallChange) should be filtered";

    publisher.shutdown();
    subscriber.shutdown();
}
