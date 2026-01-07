#include <gtest/gtest.h>
#include <opencv2/core/hal/interface.h>

#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <memory>
#include <opencv2/core/mat.hpp>
#include <opencv2/core/types.hpp>
#include <opencv2/opencv.hpp>
#include <thread>
#include <zmq.hpp>

#include "core/MQInfra/Protocol.h"
#include "core/ScreenRecorder/CaptureLayer/VideoGrabber/VideoGrabber.h"
#include "core/ScreenRecorder/ProcessLayer/FFmpegWrapper.h"
#include "core/ScreenRecorder/ProcessLayer/FrameEncoder.h"
#include "core/ScreenRecorder/ProcessLayer/ThreadSafetyQueue.h"
#include "core/ScreenRecorder/ScreenRecorder.h"
#include "zmq.h"

/**
 * @brief 辅助函数：创建一个测试用的虚假帧数据
 */
static FrameData createDummyFrame(uint64_t id, int width = 640, int height = 480) {
    FrameData data;
    // 创建一个纯色帧，便于识别
    data.frame = cv::Mat(height, width, CV_8UC3, cv::Scalar(0, 0, 255));  // 红色
    data.data = data.frame.data;
    data.width = width;
    data.height = height;
    data.format = PixelFormat::RGB24;
    data.frame_ID = id;
    data.timestamp_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                            std::chrono::steady_clock::now().time_since_epoch())
                            .count();
    return data;
}

class ScreenRecorderThreadTest : public ::testing::Test {
protected:
    void SetUp() override {
        // 清理可能遗留的测试输出文件
        std::filesystem::remove("test_encode_thread.mp4");
        std::filesystem::remove("test_keyframes.mp4");
    }

    void TearDown() override {
        std::filesystem::remove("test_encode_thread.mp4");
        std::filesystem::remove("test_keyframes.mp4");
    }
};

/**
 * @test 测试发布分发线程 (Distribution Thread)
 * 验证：ScreenRecorder -> Publish Queue -> Distribution Thread -> MQInfra::FramePublisher -> ZMQ
 */
TEST_F(ScreenRecorderThreadTest, DistributionThreadTest) {
    // 模拟下游订阅者
    zmq::context_t context(1);
    zmq::socket_t subscriber(context, ZMQ_SUB);
    subscriber.set(zmq::sockopt::subscribe, "");
    subscriber.set(zmq::sockopt::rcvtimeo, 1000);  // 1秒接收超时，用于循环
    subscriber.connect("tcp://localhost:5555");

    ScreenRecorder recorder;
    ASSERT_TRUE(recorder.startPublishing());

    // 生成并推送帧
    uint64_t testID = 12345678;
    auto frame = createDummyFrame(testID);

    // 循环发送并尝试接收，直到建立连接或达到最大尝试次数
    bool received = false;
    Protocol::FrameHeader receivedHeader;
    size_t receivedDataSize = 0;

    for (int i = 0; i < 20 && !received; ++i) {
        recorder.pushToPublishQueue(frame);

        zmq::message_t header_msg;
        auto res = subscriber.recv(header_msg, zmq::recv_flags::none);
        if (res.has_value()) {
            const auto* header = static_cast<const Protocol::FrameHeader*>(header_msg.data());
            if (header->magic_num == Protocol::FRAME_MAGIC && header->frameID == testID) {
                receivedHeader = *header;

                // 接收 Data
                zmq::message_t data_msg;
                auto resData = subscriber.recv(data_msg, zmq::recv_flags::none);
                if (resData.has_value()) {
                    receivedDataSize = data_msg.size();
                    received = true;
                }
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    ASSERT_TRUE(received)
        << "Failed to receive Frame message after multiple attempts (ZMQ Slow Joiner problem)";
    EXPECT_EQ(receivedHeader.frameID, testID);
    EXPECT_EQ(receivedHeader.width, 640);
    EXPECT_EQ(receivedHeader.data_size, 640 * 480 * 3);
    EXPECT_EQ(receivedDataSize, 640 * 480 * 3);

    recorder.stopPublishing();
}

/**
 * @test 测试关键帧元数据接收逻辑 (Receiving Thread)
 * 验证：ZMQ -> Receiving Thread -> Metadata Queue
 */
TEST_F(ScreenRecorderThreadTest, ReceivingThreadTest) {
    // 模拟上游发布者发送元数据
    zmq::context_t context(1);
    zmq::socket_t meta_pub(context, ZMQ_PUB);
    meta_pub.bind("tcp://*:5556");

    ScreenRecorder recorder;
    // 启动接收线程，连接到 localhost:5556
    // 注意：这里的 outputPath 只是传参，因为没有
    // grabber，关键帧编码器不会被初始化，所以不会有实际文件产生
    ASSERT_TRUE(recorder.startKeyFrameMetaDataReceiving("test_keyframes.mp4"));

    // 等待连接建立
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    // 准备元数据消息
    Protocol::KeyFrameMetaDataMessage msg;
    msg.header.magic_num = Protocol::METADATA_MAGIC;
    msg.header.frameID = 999;
    msg.header.Final_Score = 0.88f;
    msg.header.is_Scene_Change = false;

    zmq::message_t zmsg(sizeof(Protocol::KeyFrameMetaDataHeader));
    memcpy(zmsg.data(), &msg.header, sizeof(Protocol::KeyFrameMetaDataHeader));

    // 发送多次以确保订阅者收到（处理缓慢加入者问题）
    for (int i = 0; i < 20; ++i) {
        zmq::message_t copy;
        copy.copy(zmsg);
        meta_pub.send(copy, zmq::send_flags::none);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    // 稍微等待接收线程处理
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    // 验证逻辑：由于 recorder 内部队列是私有的且没有公开 getter，
    // 我们在此主要验证整个接收回路不崩溃且能正常关闭。
    // 在实际项目中，如果需要验证队列长度，可能需要暴露内部状态或使用 Mock。

    EXPECT_NO_THROW(recorder.stopKeyFrameMetaDataReceiving());
}

/**
 * @test 测试通用视频编码线程 (Encoding Thread)
 * 验证：Frame Queue -> FrameEncoder -> FFmpegWrapper -> File
 */
TEST_F(ScreenRecorderThreadTest, EncodingThreadTest) {
    auto queue = std::make_shared<ThreadSafetyQueue<FrameData>>(20);
    auto ffmpeg = std::make_shared<FFmpegWrapper>();

    EncoderConfig config;
    config.width = 640;
    config.height = 480;
    config.fps = 30;
    config.bitrate = 800000;
    config.outputFilePath = "test_encode_thread.mp4";
    config.codec = "libx264";
    config.enableAudio = false;

    // 如果环境缺失 ffmpeg 库，跳过测试
    if (!ffmpeg->initialize(config)) {
        GTEST_SKIP() << "FFmpeg initialize failed, skipping encoding thread test";
    }

    FrameEncoder encoder(queue, ffmpeg, config);

    std::atomic<int> progressCount{0};
    encoder.setProgressCallback([&](int64_t frames, int64_t size) { progressCount++; });

    encoder.start();

    // 连续推送 30 帧 (因为 FrameEncoder 每 30 帧才触发一次 progress callback)
    for (int i = 0; i < 30; ++i) {
        queue->push(createDummyFrame(static_cast<uint64_t>(i)), std::chrono::milliseconds(100));
        std::this_thread::sleep_for(std::chrono::milliseconds(10));  // 稍微快一点模拟录制间隔
    }

    // 给一点时间完成最后一帧的编码
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    encoder.stop();

    // 验证编码状态
    EXPECT_GE(encoder.getEncodedFrameCount(), 30);
    EXPECT_GT(progressCount.load(), 0);

    ffmpeg->finalize();

    // 检查文件是否存在且不为空
    EXPECT_TRUE(std::filesystem::exists("test_encode_thread.mp4"));
    EXPECT_GT(std::filesystem::file_size("test_encode_thread.mp4"), 0);
}
