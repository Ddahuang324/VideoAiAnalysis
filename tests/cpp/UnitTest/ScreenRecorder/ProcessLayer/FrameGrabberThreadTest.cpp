#include <gtest/gtest.h>

#include <chrono>
#include <cstdint>
#include <filesystem>
#include <iostream>
#include <memory>
#include <thread>

#include "FFmpegWrapper.h"
#include "FrameEncoder.h"
#include "FrameGrabberThread.h"
#include "GrabberFactory.h"
#include "IScreenGrabber.h"
#include "ThreadSafetyQueue.h"

// Test fixture for FrameGrabberThread
class FrameGrabberThreadTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Use AUTO grabber (likely GDI or DXGI)
        grabber = GrabberFactory::createGrabber(GrabberType::AUTO);
        // Create a queue with reasonable capacity
        queue = std::make_shared<ThreadSafetyQueue<FrameData>>(30);

        // Target 30 FPS for testing
        grabberThread = std::make_unique<FrameGrabberThread>(grabber, *queue, 30);
    }

    void TearDown() override {
        if (grabberThread && grabberThread->isRunning()) {
            grabberThread->stop();
        }
    }

    std::shared_ptr<IScreenGrabber> grabber;
    std::shared_ptr<ThreadSafetyQueue<FrameData>> queue;
    std::unique_ptr<FrameGrabberThread> grabberThread;
};

// Unit Test: Initialization
TEST_F(FrameGrabberThreadTest, Initialization) {
    EXPECT_FALSE(grabberThread->isRunning());
    EXPECT_FALSE(grabberThread->isPaused());
    EXPECT_EQ(grabberThread->getCapturedFrameCount(), 0);
    EXPECT_EQ(grabberThread->getDroppedFrameCount(), 0);
}

// Unit Test: Lifecycle (Start/Stop)
TEST_F(FrameGrabberThreadTest, Lifecycle) {
    grabberThread->start();
    EXPECT_TRUE(grabberThread->isRunning());

    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    grabberThread->stop();
    EXPECT_FALSE(grabberThread->isRunning());

    // Should have captured some frames
    EXPECT_GT(grabberThread->getCapturedFrameCount(), 0);
}

// Unit Test: Pause/Resume
TEST_F(FrameGrabberThreadTest, PauseResume) {
    grabberThread->start();
    EXPECT_TRUE(grabberThread->isRunning());

    // Run for a bit
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    int64_t countBeforePause = grabberThread->getCapturedFrameCount();

    grabberThread->pause();
    EXPECT_TRUE(grabberThread->isPaused());

    // Wait a bit, count should not increase significantly (maybe 1 pending frame)
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    int64_t countDuringPause = grabberThread->getCapturedFrameCount();

    // Allow small margin for race conditions/pending frames
    EXPECT_LE(countDuringPause - countBeforePause, 2);

    grabberThread->resume();
    EXPECT_FALSE(grabberThread->isPaused());

    // Wait a bit, count should increase
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    EXPECT_GT(grabberThread->getCapturedFrameCount(), countDuringPause);
}

// Integration Test: Record MP4 using Grabber + Encoder
TEST(IntegrationTest, RecordScreenToMP4) {
    // 1. Setup Grabber
    std::shared_ptr<IScreenGrabber> grabber = GrabberFactory::createGrabber(GrabberType::AUTO);
    ASSERT_NE(grabber, nullptr);

    // 2. Setup Shared Queue
    auto queue = std::make_shared<ThreadSafetyQueue<FrameData>>(60);

    // 3. Initialize grabber to get screen dimensions
    ASSERT_TRUE(grabber->start());

    // 4. Setup Encoder (Consumer) - get dimensions after grabber is initialized
    EncoderConfig config = encoderConfigFromGrabber(grabber.get());
    config.outputFilePath = "integration_test_record.mp4";

    // Stop grabber temporarily (FrameGrabberThread will manage it)
    grabber->stop();

    // Delete existing file if any
    if (std::filesystem::exists(config.outputFilePath)) {
        std::filesystem::remove(config.outputFilePath);
    }

    // 5. Create FrameGrabberThread (Producer) and FrameEncoder (Consumer)
    FrameGrabberThread grabberThread(grabber, *queue, config.fps);
    FrameEncoder encoder(queue, config);

    // 6. Start threads
    // Note: Start encoder first or grabber first doesn't matter much due to queue,
    // but starting encoder first ensures it's ready to consume.
    encoder.start();
    grabberThread.start();

    std::cout << "[INFO] Recording started..." << std::endl;

    // 6. Record for 3 seconds
    std::this_thread::sleep_for(std::chrono::seconds(3));

    // 7. Stop threads (Grabber first to stop producing, then Encoder to finish consuming)
    grabberThread.stop();
    // Allow encoder to process remaining frames in queue
    // In a real app we might wait for queue to be empty or send a sentinel,
    // but FrameEncoder::stop() logic should handle it (or we accept truncation).
    // Let's flush queue by waiting a bit or just stop.
    // FrameEncoder::stop() implementation usually waits for thread join.
    encoder.stop();

    std::cout << "[INFO] Recording stopped." << std::endl;

    // 8. Verify Output
    EXPECT_GT(grabberThread.getCapturedFrameCount(), 0);
    EXPECT_GT(encoder.getEncodedFrameCount(), 0);

    // Check file exists and size > 0
    std::filesystem::path p(config.outputFilePath);
    ASSERT_TRUE(std::filesystem::exists(p));
    EXPECT_GT(std::filesystem::file_size(p), 1024);  // Should be > 1KB

    std::cout << "[INFO] Output file size: " << std::filesystem::file_size(p) << " bytes"
              << std::endl;
}
