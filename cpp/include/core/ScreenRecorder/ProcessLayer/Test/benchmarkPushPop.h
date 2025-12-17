
#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <thread>
#include <vector>

#include "ThreadSafetyQueue.h"

// Realistic multi-threaded benchmark simulating producer-consumer pattern
inline void benchmarkPushPop() {
    // Use realistic queue size (30 frames = ~240MB buffer for 1080p BGRA)
    // This is typical for video processing to balance memory and latency
    const size_t QUEUE_SIZE = 30;
    const int TOTAL_FRAMES = 1000;

    ThreadSafetyQueue<std::vector<uint8_t>> queue(QUEUE_SIZE);

    // Simulate 1080p BGRA frame (1920x1080x4 bytes = ~8MB per frame)
    std::vector<uint8_t> frame(1920 * 1080 * 4);

    std::atomic<int> push_success{0};
    std::atomic<int> push_timeout{0};
    std::atomic<int> pop_success{0};
    std::atomic<int> pop_timeout{0};

    // Producer thread: simulates screen capture
    std::thread producer([&]() {
        auto start = std::chrono::high_resolution_clock::now();

        for (int i = 0; i < TOTAL_FRAMES; ++i) {
            // Simulate 60 FPS capture rate (~16.67ms per frame)
            std::this_thread::sleep_for(std::chrono::microseconds(16670));

            if (queue.push(frame, std::chrono::milliseconds(100))) {
                push_success++;
            } else {
                push_timeout++;
                std::cout << "WARNING: Push timeout at frame " << i
                          << " (queue full, encoder too slow)" << std::endl;
            }
        }

        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

        std::cout << "\n=== Producer Stats ===" << std::endl;
        std::cout << "Total time: " << duration.count() << " ms" << std::endl;
        std::cout << "Successful pushes: " << push_success << std::endl;
        std::cout << "Timeout pushes: " << push_timeout << std::endl;
        std::cout << "Actual FPS: " << (push_success * 1000.0) / duration.count() << std::endl;
    });

    // Consumer thread: simulates video encoding
    std::thread consumer([&]() {
        std::vector<uint8_t> popped_frame;
        auto start = std::chrono::high_resolution_clock::now();

        while (pop_success < TOTAL_FRAMES) {
            if (queue.pop(popped_frame, std::chrono::milliseconds(100))) {
                pop_success++;

                // Simulate encoding time (varies: 10-30ms for hardware encoding)
                // This creates realistic backpressure on the queue
                std::this_thread::sleep_for(std::chrono::milliseconds(20));
            } else {
                pop_timeout++;
            }
        }

        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

        std::cout << "\n=== Consumer Stats ===" << std::endl;
        std::cout << "Total time: " << duration.count() << " ms" << std::endl;
        std::cout << "Successful pops: " << pop_success << std::endl;
        std::cout << "Timeout pops: " << pop_timeout << std::endl;
        std::cout << "Processing FPS: " << (pop_success * 1000.0) / duration.count() << std::endl;
    });

    producer.join();
    consumer.join();

    std::cout << "\n=== Queue Performance ===" << std::endl;
    std::cout << "Queue size: " << QUEUE_SIZE << " frames" << std::endl;
    std::cout << "Final queue size: " << queue.size() << " frames" << std::endl;
    std::cout << "Frame loss rate: " << (push_timeout * 100.0) / TOTAL_FRAMES << "%" << std::endl;

    if (push_timeout > 0) {
        std::cout << "\n[!] WARNING: Frame drops detected!" << std::endl;
        std::cout << "    Consider: 1) Faster encoding, 2) Larger queue, 3) Lower capture FPS"
                  << std::endl;
    } else {
        std::cout << "\n[OK] No frame drops - queue size is adequate!" << std::endl;
    }
}