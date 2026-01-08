#include <gtest/gtest.h>

#include <chrono>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <opencv2/core/mat.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/opencv.hpp>
#include <ostream>
#include <string>
#include <thread>
#include <vector>

#include "core/KeyFrame/Foundation/DataConverter.h"
#include "core/MQInfra/FramePublisher.h"
#include "core/MQInfra/FrameSubscriber.h"
#include "core/MQInfra/KeyFrameMetaDataPublisher.h"
#include "core/MQInfra/KeyFrameMetaDataSubscriber.h"
#include "core/MQInfra/Protocol.h"

#ifndef TEST_ASSETS_DIR
#    define TEST_ASSETS_DIR "tests/cpp/UnitTest/KeyFrame/TestImage"
#endif

class ZMQConnectionTest : public ::testing::Test {
protected:
    void SetUp() override { std::this_thread::sleep_for(std::chrono::milliseconds(100)); }
    void TearDown() override {}
};

TEST_F(ZMQConnectionTest, FrameImageTransmissionTest) {
    MQInfra::FramePublisher publisher;
    MQInfra::FrameSubscriber subscriber;
    const std::string endpoint = "tcp://127.0.0.1:5557";

    ASSERT_TRUE(publisher.initialize(endpoint));
    ASSERT_TRUE(subscriber.initialize(endpoint));

    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    std::string imagePath = std::string(TEST_ASSETS_DIR) + "/1-anytype.png";
    cv::Mat img = KeyFrame::DataConverter::readImage(imagePath);
    ASSERT_FALSE(img.empty()) << "Failed to load image: " << imagePath;

    // 2. 准备测试数据
    std::cout << "Successfully loaded image: " << imagePath << " [" << img.cols << "x" << img.rows
              << "]" << std::endl;

    Protocol::FrameMessage sendMsg;
    sendMsg.header.frameID = 1001;
    sendMsg.header.width = static_cast<uint32_t>(img.cols);
    sendMsg.header.height = static_cast<uint32_t>(img.rows);
    sendMsg.header.channels = static_cast<uint8_t>(img.channels());
    sendMsg.header.data_size = static_cast<uint32_t>(img.total() * img.elemSize());

    sendMsg.image_data.assign(img.data, img.data + sendMsg.header.data_size);

    // 正确的 CRC 计算逻辑：包括 Header + Data，并且最后与 0xFFFFFFFF 异或
    uint32_t computed_crc = Protocol::calculateCrc32(&sendMsg.header, sizeof(sendMsg.header));
    computed_crc = Protocol::calculateCrc32(sendMsg.image_data.data(), sendMsg.image_data.size(),
                                            computed_crc);
    sendMsg.crc32 = computed_crc ^ 0xFFFFFFFF;

    bool received = false;
    int maxAttempts = 100;
    for (int i = 0; i < maxAttempts && !received; ++i) {
        publisher.publish(sendMsg);

        auto recvOpt = subscriber.receiveFrame(50);
        if (recvOpt.has_value()) {
            const auto& recvMsg = recvOpt.value();
            if (recvMsg.header.frameID == 1001) {
                EXPECT_EQ(recvMsg.header.width, sendMsg.header.width);
                EXPECT_EQ(recvMsg.header.height, sendMsg.header.height);
                EXPECT_EQ(recvMsg.image_data.size(), sendMsg.image_data.size());
                EXPECT_EQ(recvMsg.crc32, sendMsg.crc32);
                EXPECT_EQ(
                    std::memcmp(img.data, recvMsg.image_data.data(), sendMsg.header.data_size), 0);

                received = true;
                std::cout << "Successfully received frame at attempt: " << i + 1 << std::endl;
            }
        }
        if (!received) {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
    }

    ASSERT_TRUE(received) << "Failed to receive image data via ZMQ";

    publisher.shutdown();
    subscriber.shutdown();
}

TEST_F(ZMQConnectionTest, MetaDataTransmissionTest) {
    MQInfra::KeyFrameMetaDataPublisher publisher;
    MQInfra::KeyFrameMetaDataSubscriber subscriber;
    const std::string endpoint = "tcp://127.0.0.1:5558";

    ASSERT_TRUE(publisher.initialize(endpoint));
    ASSERT_TRUE(subscriber.initialize(endpoint));

    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    Protocol::KeyFrameMetaDataMessage sendMeta;
    sendMeta.header.frameID = 2002;
    sendMeta.header.Final_Score = 0.95f;
    sendMeta.header.Sence_Score = 0.8f;
    sendMeta.header.Motion_Score = 0.7f;
    sendMeta.header.Text_Score = 0.9f;
    sendMeta.header.is_Scene_Change = 1;

    bool received = false;
    for (int i = 0; i < 50 && !received; ++i) {
        publisher.publish(sendMeta);

        auto recvOpt = subscriber.receiveMetaData(50);
        if (recvOpt.has_value()) {
            const auto& recvMeta = recvOpt.value();
            if (recvMeta.header.frameID == 2002) {
                EXPECT_FLOAT_EQ(recvMeta.header.Final_Score, 0.95f);
                EXPECT_EQ(recvMeta.header.is_Scene_Change, 1);
                received = true;
                std::cout << "Successfully received metadata at attempt: " << i + 1 << std::endl;
            }
        }
        if (!received) {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
    }

    ASSERT_TRUE(received) << "Failed to receive metadata via ZMQ";

    publisher.shutdown();
    subscriber.shutdown();
}
