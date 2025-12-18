#include <gtest/gtest.h>

#include <iostream>
#include <memory>

#include "GrabberFactory.h"
#include "IScreenGrabber.h"

class CaptureTest : public ::testing::Test {
protected:
    void SetUp() override { grabber = GrabberFactory::createGrabber(GrabberType::AUTO); }

    void TearDown() override {
        if (grabber) {
            grabber->stop();
        }
    }

    std::unique_ptr<IScreenGrabber> grabber;
};

TEST_F(CaptureTest, InitializeGrabber) {
    ASSERT_NE(grabber, nullptr);
    EXPECT_TRUE(grabber->start());
    EXPECT_TRUE(grabber->isRunning());
    EXPECT_FALSE(grabber->isPaused());

    int width = grabber->getWidth();
    int height = grabber->getHeight();
    std::cout << "[INFO] Screen resolution: " << width << "x" << height << std::endl;
    EXPECT_GT(width, 0);
    EXPECT_GT(height, 0);
}

TEST_F(CaptureTest, CaptureSingleFrame) {
    ASSERT_NE(grabber, nullptr);
    ASSERT_TRUE(grabber->start());

    // 等待一小会儿确保资源就绪（虽然 GDI 通常很快）
    FrameData frame = grabber->CaptureFrame(100);

    EXPECT_NE(frame.data, nullptr);
    EXPECT_GT(frame.width, 0);
    EXPECT_GT(frame.height, 0);
    EXPECT_NE(frame.format, PixelFormat::UNKNOWN);

    std::cout << "[INFO] Captured frame: " << frame.width << "x" << frame.height
              << ", format: " << (int)frame.format << std::endl;
}

TEST_F(CaptureTest, CaptureMultipleFrames) {
    ASSERT_NE(grabber, nullptr);
    ASSERT_TRUE(grabber->start());

    for (int i = 0; i < 5; ++i) {
        FrameData frame = grabber->CaptureFrame(100);
        EXPECT_NE(frame.data, nullptr);
        std::cout << "[INFO] Captured frame " << i << " at " << frame.timestamp_ms << "ms"
                  << std::endl;
    }
}

TEST_F(CaptureTest, PauseResume) {
    ASSERT_NE(grabber, nullptr);
    ASSERT_TRUE(grabber->start());

    grabber->pause();
    EXPECT_TRUE(grabber->isPaused());

    // 暂停时捕获应该返回空数据
    FrameData frame = grabber->CaptureFrame(10);
    EXPECT_EQ(frame.data, nullptr);

    grabber->resume();
    EXPECT_FALSE(grabber->isPaused());

    frame = grabber->CaptureFrame(100);
    EXPECT_NE(frame.data, nullptr);
}
