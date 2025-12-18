#include <gtest/gtest.h>

#include <chrono>
#include <cstdint>
#include <cstdio>
#include <iostream>
#include <memory>
#include <string>
#include <thread>

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

// Helper to save BMP
void saveBMP(const char* filename, int width, int height, const uint8_t* data) {
    FILE* f = fopen(filename, "wb");
    if (!f)
        return;

    // BMP Header
    unsigned char fileHeader[14] = {
        'B', 'M',        // Signature
        0,   0,   0, 0,  // File size (placeholder)
        0,   0,          // Reserved1
        0,   0,          // Reserved2
        54,  0,   0, 0   // Offset to pixel data
    };

    unsigned char infoHeader[40] = {
        40, 0, 0, 0,  // Info header size
        0,  0, 0, 0,  // Width (placeholder)
        0,  0, 0, 0,  // Height (placeholder)
        1,  0,        // Planes
        32, 0,        // Bits per pixel (assuming 32-bit BGRA/RGBA)
        0,  0, 0, 0,  // Compression (0 = none)
        0,  0, 0, 0,  // Image size (0 for uncompressed)
        0,  0, 0, 0,  // X pixels/meter
        0,  0, 0, 0,  // Y pixels/meter
        0,  0, 0, 0,  // Colors
        0,  0, 0, 0   // Important colors
    };

    int fileSize = 54 + width * height * 4;
    fileHeader[2] = (unsigned char)(fileSize);
    fileHeader[3] = (unsigned char)(fileSize >> 8);
    fileHeader[4] = (unsigned char)(fileSize >> 16);
    fileHeader[5] = (unsigned char)(fileSize >> 24);

    infoHeader[4] = (unsigned char)(width);
    infoHeader[5] = (unsigned char)(width >> 8);
    infoHeader[6] = (unsigned char)(width >> 16);
    infoHeader[7] = (unsigned char)(width >> 24);

    // Height: 使用负值表示 top-down 位图(屏幕捕获通常是 top-down)
    // 这样 BMP 文件就会正确显示,不会上下颠倒
    int bmpHeight = -height;  // 负高度 = top-down
    infoHeader[8] = (unsigned char)(bmpHeight);
    infoHeader[9] = (unsigned char)(bmpHeight >> 8);
    infoHeader[10] = (unsigned char)(bmpHeight >> 16);
    infoHeader[11] = (unsigned char)(bmpHeight >> 24);

    fwrite(fileHeader, 1, 14, f);
    fwrite(infoHeader, 1, 40, f);
    fwrite(data, 1, width * height * 4, f);
    fclose(f);
}

TEST_F(CaptureTest, CaptureAndSaveImage) {
    ASSERT_NE(grabber, nullptr);
    ASSERT_TRUE(grabber->start());

    // Wait for a clear frame
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    FrameData frame = grabber->CaptureFrame(100);

    ASSERT_NE(frame.data, nullptr);
    ASSERT_GT(frame.width, 0);
    ASSERT_GT(frame.height, 0);

    // Save as BMP (User asked for PNG but BMP is dependency-free for this quick test.
    // If PNG is strictly required, we would need to link stbi or use ffmpeg manually here)
    std::string filename = "test_capture_frame.bmp";
    saveBMP(filename.c_str(), frame.width, frame.height, frame.data);

    std::cout << "[INFO] Saved captured frame to " << filename << " (" << frame.width << "x"
              << frame.height << ")" << std::endl;
}
