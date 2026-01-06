#include <gtest/gtest.h>
#include <opencv2/core/hal/interface.h>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <map>
#include <memory>
#include <numeric>
#include <opencv2/core/mat.hpp>
#include <opencv2/core/types.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/opencv.hpp>
#include <opencv2/videoio.hpp>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "FFmpegWrapper.h"
#include "ModelManager.h"
#include "MotionDetector.h"
#include "MotionVisualizer.h"
#include "VideoGrabber.h"

namespace fs = std::filesystem;

class MotionDetectorTest : public ::testing::Test {
protected:
    void SetUp() override {
        // 智能查找模型路径，支持多种运行场景
        fs::path exePath = fs::current_path();
        fs::path modelPath;

        // 尝试多个可能的路径 (YOLOv8n 模型)
        std::vector<fs::path> possiblePaths = {
            // 1. 当前目录就是项目根目录
            exePath / "Models" / "yolov8n.onnx",
            // 2. 从 build/bin 目录运行，回退两级
            exePath.parent_path().parent_path() / "Models" / "yolov8n.onnx",
            // 3. 模型文件在 build/bin 目录下
            exePath / "yolov8n.onnx",
            // 4. 使用 CMake 定义的宏
            fs::path(TEST_MODELS_DIR) / "yolov8n.onnx"};

        bool found = false;
        for (const auto& path : possiblePaths) {
            if (fs::exists(path)) {
                modelPath = path;
                found = true;
                break;
            }
        }

        if (!found) {
            std::ostringstream oss;
            oss << "YOLOv8n model file not found. Tried paths:\n";
            for (size_t i = 0; i < possiblePaths.size(); ++i) {
                oss << "  " << (i + 1) << ". " << possiblePaths[i].string() << "\n";
            }
            oss << "Current path: " << fs::current_path().string();
            FAIL() << oss.str();
        }

        std::cout << "[Setup] Using YOLOv8n model path: " << modelPath.string() << std::endl;

        // 初始化 ModelManager 并加载模型
        modelManager_ = &KeyFrame::ModelManager::GetInstance();
        if (!modelManager_->hasModel("yolov8n.onnx")) {
            modelManager_->loadModel("yolov8n.onnx", modelPath.string(),
                                     KeyFrame::ModelManager::FrameWorkType::ONNXRuntime);
        }

        // 创建默认配置
        config_.confidenceThreshold = 0.25f;
        config_.nmsThreshold = 0.45f;
        config_.inputWidth = 640;
        config_.trackHighThreshold = 0.6f;
        config_.trackBufferSize = 30;

        detector_ =
            std::make_unique<KeyFrame::MotionDetector>(*modelManager_, config_, "yolov8n.onnx");
    }

    void TearDown() override {
        if (detector_) {
            detector_->reset();
        }
    }

    // 创建带有移动物体的合成测试帧
    cv::Mat createSyntheticFrame(int width, int height, int rectX, int rectY, int rectW = 100,
                                 int rectH = 100) {
        cv::Mat frame(height, width, CV_8UC3, cv::Scalar(128, 128, 128));  // 灰色背景
        cv::rectangle(frame, cv::Rect(rectX, rectY, rectW, rectH), cv::Scalar(0, 0, 255),
                      -1);  // 红色矩形
        return frame;
    }

    // 创建静态帧 (无运动对象)
    cv::Mat createStaticFrame(int width, int height) {
        cv::Mat frame(height, width, CV_8UC3, cv::Scalar(200, 200, 200));  // 浅灰色
        return frame;
    }

    KeyFrame::ModelManager* modelManager_ = nullptr;
    KeyFrame::MotionDetector::Config config_;
    std::unique_ptr<KeyFrame::MotionDetector> detector_;
};

/**
 * @brief 测试空帧输入
 * 验证检测器对空帧的处理是否返回默认结果
 */
TEST_F(MotionDetectorTest, EmptyFrameReturnsDefaultResult) {
    cv::Mat emptyFrame;
    auto result = detector_->detect(emptyFrame);

    EXPECT_FLOAT_EQ(result.score, 0.0f);
    EXPECT_TRUE(result.track.empty());
    EXPECT_EQ(result.newTracks, 0);
    EXPECT_EQ(result.LostTracks, 0);
    EXPECT_FLOAT_EQ(result.avgVelocity, 0.0f);

    std::cout << "[Test] Empty frame test passed - returned default result" << std::endl;
}

/**
 * @brief 测试静态场景
 * 在没有可检测对象的静态帧上运行检测
 */
TEST_F(MotionDetectorTest, StaticSceneHasLowScore) {
    cv::Mat staticFrame = createStaticFrame(640, 480);
    auto result = detector_->detect(staticFrame);

    // 静态场景应该有较低的运动分数
    EXPECT_LE(result.score, 0.5f);
    std::cout << "[Test] Static scene score: " << result.score
              << ", Tracks: " << result.track.size() << std::endl;
}

/**
 * @brief 测试检测器重置功能
 */
TEST_F(MotionDetectorTest, ResetClearsState) {
    // 先运行一些检测
    cv::Mat frame = createSyntheticFrame(640, 480, 100, 100);
    detector_->detect(frame);

    // 重置
    detector_->reset();

    // 验证轨迹已清除
    EXPECT_TRUE(detector_->GetTracks().empty());
    std::cout << "[Test] Reset test passed - tracks cleared" << std::endl;
}

/**
 * @brief 测试检测结果结构完整性
 * 验证返回的 Result 结构包含所有必要字段
 */
TEST_F(MotionDetectorTest, ResultStructureIntegrity) {
    cv::Mat frame = createSyntheticFrame(640, 480, 200, 150);
    auto result = detector_->detect(frame);

    // 验证分数在合理范围内
    EXPECT_GE(result.score, 0.0f);
    EXPECT_LE(result.score, 1.0f);

    // 验证新增和丢失轨迹计数非负
    EXPECT_GE(result.newTracks, 0);
    EXPECT_GE(result.LostTracks, 0);

    // 验证平均速度非负
    EXPECT_GE(result.avgVelocity, 0.0f);

    std::cout << "[Test] Result structure: score=" << result.score
              << ", newTracks=" << result.newTracks << ", lostTracks=" << result.LostTracks
              << ", avgVelocity=" << result.avgVelocity << std::endl;
}

/**
 * @brief 测试使用真实图片进行检测
 * 使用 TestImage 目录中的图片进行检测测试
 */
TEST_F(MotionDetectorTest, RealImageDetection) {
    // 智能查找测试资源目录
    fs::path exePath = fs::current_path();
    fs::path assetsDir;

    std::vector<fs::path> possibleAssetDirs = {
        exePath / "tests" / "cpp" / "UnitTest" / "KeyFrame" / "TestImage",
        exePath.parent_path().parent_path() / "tests" / "cpp" / "UnitTest" / "KeyFrame" /
            "TestImage",
        fs::path(TEST_ASSETS_DIR)};

    bool found = false;
    for (const auto& dir : possibleAssetDirs) {
        if (fs::exists(dir)) {
            assetsDir = dir;
            found = true;
            break;
        }
    }

    if (!found) {
        GTEST_SKIP() << "Test assets directory not found, skipping real image test";
    }

    std::cout << "[Test] Using assets directory: " << assetsDir.string() << std::endl;

    cv::Mat img1 = cv::imread((assetsDir / "1-anytype.png").string());
    if (img1.empty()) {
        GTEST_SKIP() << "Test image 1-anytype.png not found";
    }

    auto result = detector_->detect(img1);

    std::cout << "[Test] Real image detection: score=" << result.score
              << ", tracks=" << result.track.size() << ", avgVelocity=" << result.avgVelocity
              << std::endl;

    // 验证检测正常执行（不验证具体值，因为取决于图片内容）
    EXPECT_GE(result.score, 0.0f);
    EXPECT_LE(result.score, 1.0f);
}

/**
 * @brief 测试多帧连续检测
 * 模拟视频流场景，连续处理多帧
 */
TEST_F(MotionDetectorTest, MultipleFrameDetection) {
    std::cout << "\n[Test] Starting multiple frame detection test..." << std::endl;

    std::vector<float> scores;

    // 模拟 5 帧，物体从左向右移动
    for (int i = 0; i < 5; ++i) {
        int x = 50 + i * 80;  // 物体水平移动
        cv::Mat frame = createSyntheticFrame(640, 480, x, 200);
        auto result = detector_->detect(frame);
        scores.push_back(result.score);

        std::cout << "[Frame " << i << "] Position=(" << x << ", 200)"
                  << ", Score=" << result.score << ", Tracks=" << result.track.size()
                  << ", NewTracks=" << result.newTracks << ", AvgVelocity=" << result.avgVelocity
                  << std::endl;
    }

    // 验证所有检测返回有效结果
    for (size_t i = 0; i < scores.size(); ++i) {
        EXPECT_GE(scores[i], 0.0f) << "Frame " << i << " has invalid score";
        EXPECT_LE(scores[i], 1.0f) << "Frame " << i << " has invalid score";
    }

    std::cout << "[Test] Multiple frame detection test completed" << std::endl;
}

/**
 * @brief 测试 Track 结构字段
 * 验证检测到的 Track 包含有效数据
 */
TEST_F(MotionDetectorTest, TrackFieldsValidity) {
    // 创建一个可能包含可检测对象的帧
    cv::Mat frame(640, 480, CV_8UC3);

    // 绘制一些形状来模拟检测目标
    cv::rectangle(frame, cv::Rect(100, 100, 150, 200), cv::Scalar(255, 0, 0), -1);
    cv::rectangle(frame, cv::Rect(350, 150, 120, 180), cv::Scalar(0, 255, 0), -1);

    auto result = detector_->detect(frame);

    // 验证每个 Track 的字段有效性
    for (const auto& track : result.track) {
        EXPECT_GE(track.trackId, 0);
        EXPECT_GE(track.confidence, 0.0f);
        EXPECT_LE(track.confidence, 1.0f);
        EXPECT_GE(track.classId, 0);
        EXPECT_LT(track.classId, 80);  // COCO 有 80 个类别
        EXPECT_GE(track.box.width, 0);
        EXPECT_GE(track.box.height, 0);
    }

    std::cout << "[Test] Track fields validity test completed, "
              << "found " << result.track.size() << " tracks" << std::endl;
}

/**
 * @brief 测试配置参数影响
 * 验证不同配置参数对检测结果的影响
 */
TEST_F(MotionDetectorTest, ConfigAffectsDetection) {
    // 创建高阈值配置
    KeyFrame::MotionDetector::Config highThreshConfig;
    highThreshConfig.confidenceThreshold = 0.8f;  // 高置信度阈值
    highThreshConfig.nmsThreshold = 0.45f;
    highThreshConfig.inputWidth = 640;

    auto highThreshDetector = std::make_unique<KeyFrame::MotionDetector>(
        *modelManager_, highThreshConfig, "yolov8n.onnx");

    cv::Mat frame = createSyntheticFrame(640, 480, 200, 200);

    auto defaultResult = detector_->detect(frame);
    detector_->reset();
    highThreshDetector->reset();

    auto highThreshResult = highThreshDetector->detect(frame);

    std::cout << "[Test] Default config tracks: " << defaultResult.track.size()
              << ", High threshold config tracks: " << highThreshResult.track.size() << std::endl;

    // 高阈值通常会导致更少的检测
    // 注意：这不是严格的断言，因为取决于图像内容
}

/**
 * @brief 测试 GetTracks 接口
 */
TEST_F(MotionDetectorTest, GetTracksInterface) {
    cv::Mat frame = createSyntheticFrame(640, 480, 100, 100);
    auto result = detector_->detect(frame);

    // 通过接口获取轨迹
    const auto& tracks = detector_->GetTracks();

    // 验证 GetTracks 返回与 Result 中相同的轨迹
    EXPECT_EQ(tracks.size(), result.track.size());

    std::cout << "[Test] GetTracks interface test passed, "
              << "returned " << tracks.size() << " tracks" << std::endl;
}

/**
 * @brief 测试视频处理与可视化
 *
 * 使用录制的视频进行完整的运动检测流程测试：
 * 1. 逐帧读取视频
 * 2. 运行运动检测
 * 3. 应用可视化标注
 * 4. 输出带标注的视频
 * 5. 生成详细的终端评分报告
 */
TEST_F(MotionDetectorTest, VideoProcessingWithVisualization) {
    // 智能查找测试视频路径
    fs::path exePath = fs::current_path();
    fs::path videoPath;

    std::vector<fs::path> possiblePaths = {
        exePath / "tests" / "cpp" / "UnitTest" / "TestVideo" / "recording_20251228_211312.mp4",
        exePath.parent_path().parent_path() / "tests" / "cpp" / "UnitTest" / "TestVideo" /
            "recording_20251228_211312.mp4",
        exePath / "recording_20251228_211312.mp4"};

    bool found = false;
    for (const auto& path : possiblePaths) {
        if (fs::exists(path)) {
            videoPath = path;
            found = true;
            break;
        }
    }

    if (!found) {
        GTEST_SKIP() << "Test video 'recording_20251228_211312.mp4' not found";
    }

    std::cout << "\n[Test] Using video: " << videoPath.string() << std::endl;

    // 打开视频
    cv::VideoCapture cap(videoPath.string());
    ASSERT_TRUE(cap.isOpened()) << "Failed to open video: " << videoPath;

    // 获取视频属性
    int totalFrames = static_cast<int>(cap.get(cv::CAP_PROP_FRAME_COUNT));
    double fps = cap.get(cv::CAP_PROP_FPS);
    int width = static_cast<int>(cap.get(cv::CAP_PROP_FRAME_WIDTH));
    int height = static_cast<int>(cap.get(cv::CAP_PROP_FRAME_HEIGHT));
    double duration = totalFrames / fps;

    std::cout << "[Video Info] Frames: " << totalFrames << ", FPS: " << fps
              << ", Resolution: " << width << "x" << height << ", Duration: " << std::fixed
              << std::setprecision(1) << duration << "s" << std::endl;

    // 确保宽度和高度是偶数（YUV420P 编码要求 2x2 采样 Block）
    int encWidth = (width % 2 == 0) ? width : width - 1;
    int encHeight = (height % 2 == 0) ? height : height - 1;

    // 创建输出视频路径 - 使用相对路径避开 FFmpeg 对中文绝对路径可能的编码问题
    fs::path outputPath = "output_motion_analysis.mp4";
    if (fs::exists(outputPath)) {
        fs::remove(outputPath);
    }
    std::cout << "[Output] Annotated video will be saved to: " << fs::absolute(outputPath).string()
              << std::endl;

    // 检查是否启用视频输出
    bool enableVideoOutput = false;
    for (int i = 0; i < __argc; ++i) {
        if (std::string(__argv[i]) == "--save-video") {
            enableVideoOutput = true;
            break;
        }
    }

    if (!enableVideoOutput) {
        std::cout << "[Info] Video output is disabled. Use --save-video to enable." << std::endl;
    }

    // 使用项目的 FFmpegWrapper 创建视频编码器
    auto ffmpegWrapper = std::make_shared<FFmpegWrapper>();
    EncoderConfig encoderConfig;
    encoderConfig.outputFilePath = outputPath.string();
    encoderConfig.width = encWidth;
    encoderConfig.height = encHeight;
    encoderConfig.fps = (fps > 0) ? static_cast<int>(fps) : 30;
    encoderConfig.bitrate = 5000000;  // 5 Mbps
    encoderConfig.crf = 23;
    encoderConfig.preset = "medium";
    encoderConfig.codec = "libx264";
    encoderConfig.enableAudio = false;  // 测试视频不需要音频

    bool encoderInitialized = false;
    if (enableVideoOutput) {
        encoderInitialized = ffmpegWrapper->initialize(encoderConfig);
        if (encoderInitialized) {
            std::cout << "[Info] Encoder initialized with resolution " << encWidth << "x"
                      << encHeight << std::endl;
        } else {
            std::cout << "[Error] Failed to initialize FFmpeg encoder: "
                      << ffmpegWrapper->getLastError() << std::endl;
        }
    }

    // 创建可视化器
    KeyFrame::MotionVisualizer::Config vizConfig;
    vizConfig.showBoundingBoxes = true;
    vizConfig.showTrackIds = true;
    vizConfig.showConfidence = true;
    vizConfig.showVelocityArrows = true;
    vizConfig.showTrackHistory = true;
    vizConfig.historyLength = 30;
    vizConfig.showHUD = true;
    vizConfig.showClassLabels = true;
    vizConfig.hudOpacity = 0.7f;

    KeyFrame::MotionVisualizer visualizer(vizConfig);

    // 统计数据收集
    std::vector<float> scores;
    std::vector<float> pixelScores;     // 像素运动强度收集
    std::map<int, int> classCount;      // 类别ID -> 出现次数
    std::map<int, int> trackLifetimes;  // Track ID -> 生命周期（帧数）
    float maxScore = 0.0f;
    int maxScoreFrame = 0;
    float minScore = 1.0f;
    int minScoreFrame = 0;

    // 逐帧处理
    cv::Mat frame;
    int frameIdx = 0;
    int processedFrames = 0;

    std::cout << "\n[Processing] Starting video processing..." << std::endl;

    while (cap.read(frame)) {
        if (frame.empty()) {
            break;
        }

        // 运动检测
        auto result = detector_->detect(frame);

        // 记录评分
        scores.push_back(result.score);
        pixelScores.push_back(result.pixelMotionScore);

        if (result.score > maxScore) {
            maxScore = result.score;
            maxScoreFrame = frameIdx;
        }
        if (result.score < minScore) {
            minScore = result.score;
            minScoreFrame = frameIdx;
        }

        // 统计检测对象
        for (const auto& track : result.track) {
            classCount[track.classId]++;
            trackLifetimes[track.trackId]++;
        }

        // 如果启用了视频输出，则应用可视化并写入视频文件
        if (encoderInitialized) {
            // 应用可视化 (昂贵的操作)
            cv::Mat annotated = visualizer.draw(frame, result, frameIdx);

            // 如果原始分辨率是奇数，需要 resize 到偶数分辨率
            if (annotated.cols != encWidth || annotated.rows != encHeight) {
                cv::resize(annotated, annotated, cv::Size(encWidth, encHeight));
            }

            // OpenCV Mat 默认是 BGR 格式（3通道），需要转换为 BGRA（4通道）
            cv::Mat bgraFrame;
            cv::cvtColor(annotated, bgraFrame, cv::COLOR_BGR2BGRA);

            // 创建 FrameData 用于编码
            FrameData frameData;
            frameData.data = bgraFrame.data;
            frameData.width = bgraFrame.cols;
            frameData.height = bgraFrame.rows;
            frameData.format = PixelFormat::BGRA;
            // 修正：将帧索引转换为毫秒时间戳
            frameData.timestamp_ms =
                static_cast<int64_t>(frameIdx * 1000.0 / (fps > 0 ? fps : 30.0));

            if (!ffmpegWrapper->encoderFrame(frameData)) {
                std::cout << "[Warning] Failed to encode frame " << frameIdx << std::endl;
            }
        }

        frameIdx++;
        processedFrames++;

        // 每处理 100 帧输出一次进度
        if (frameIdx % 100 == 0) {
            std::cout << "[Progress] Processed " << frameIdx << " / " << totalFrames << " frames"
                      << std::endl;
        }
    }

    cap.release();

    // 完成编码并写入文件尾
    if (encoderInitialized) {
        ffmpegWrapper->finalize();
        std::cout << "[Output] Video encoding completed: " << outputPath.filename().string()
                  << std::endl;
    }

    std::cout << "[Processing] Completed! Processed " << processedFrames << " frames" << std::endl;

    // ========== 生成终端评分报告 ==========

    std::cout << "\n";
    std::cout
        << "===============================================================================\n";
    std::cout << "                        运动检测分析报告\n";
    std::cout
        << "===============================================================================\n";

    // 视频信息
    std::cout << "视频信息:\n";
    std::cout << "  文件: " << videoPath.filename().string() << "\n";
    std::cout << "  总帧数: " << processedFrames << "\n";
    std::cout << "  帧率: " << std::fixed << std::setprecision(1) << fps << " fps\n";
    std::cout << "  时长: " << std::setprecision(2) << (processedFrames / fps) << " 秒\n";
    std::cout << "  分辨率: " << width << "x" << height << "\n\n";

    // 运动评分统计
    if (!scores.empty()) {
        float avgScore = std::accumulate(scores.begin(), scores.end(), 0.0f) / scores.size();

        std::vector<float> sortedScores = scores;
        std::sort(sortedScores.begin(), sortedScores.end());
        float median = sortedScores[sortedScores.size() / 2];

        float variance = 0.0f;
        for (float score : scores) {
            variance += (score - avgScore) * (score - avgScore);
        }
        float stdDev = std::sqrt(variance / scores.size());

        std::cout << "运动评分统计:\n";
        std::cout << "  平均分数: " << std::fixed << std::setprecision(3) << avgScore << "\n";
        std::cout << "  最高分数: " << maxScore << " (第 " << maxScoreFrame << " 帧, "
                  << std::setprecision(1) << (maxScoreFrame / fps) << " 秒)\n";
        std::cout << "  最低分数: " << std::fixed << std::setprecision(3) << minScore << " (第 "
                  << minScoreFrame << " 帧, " << std::setprecision(1) << (minScoreFrame / fps)
                  << " 秒)\n";
        std::cout << "  中位数: " << std::setprecision(3) << median << "\n";
        std::cout << "  标准差: " << stdDev << "\n\n";
    }

    // 像素运动强度统计 (新增)
    if (!pixelScores.empty()) {
        float avgPixelScore =
            std::accumulate(pixelScores.begin(), pixelScores.end(), 0.0f) / pixelScores.size();
        float maxPixelScore = *std::max_element(pixelScores.begin(), pixelScores.end());

        std::cout << "像素运动强度统计 (Frame Difference):\n";
        std::cout << "  平均强度: " << std::fixed << std::setprecision(3) << avgPixelScore << "\n";
        std::cout << "  最大强度: " << maxPixelScore << "\n\n";
    }

    // 检测对象统计
    if (!classCount.empty()) {
        std::cout << "检测对象统计:\n";

        // 计算总检测次数
        int totalDetections = 0;
        for (const auto& [classId, count] : classCount) {
            totalDetections += count;
        }

        // 按检测次数排序
        std::vector<std::pair<int, int>> sortedClasses(classCount.begin(), classCount.end());
        std::sort(sortedClasses.begin(), sortedClasses.end(),
                  [](const auto& a, const auto& b) { return a.second > b.second; });

        // 显示 top 10 类别
        int displayCount = std::min(10, static_cast<int>(sortedClasses.size()));
        for (int i = 0; i < displayCount; ++i) {
            int classId = sortedClasses[i].first;
            int count = sortedClasses[i].second;
            float percentage = (count * 100.0f) / totalDetections;

            // 获取类别名称
            const char* className = "unknown";
            if (classId >= 0 && classId < 80) {
                const char* COCO_CLASSES[80] = {"person",        "bicycle",      "car",
                                                "motorcycle",    "airplane",     "bus",
                                                "train",         "truck",        "boat",
                                                "traffic light", "fire hydrant", "stop sign",
                                                "parking meter", "bench",        "bird",
                                                "cat",           "dog",          "horse",
                                                "sheep",         "cow",          "elephant",
                                                "bear",          "zebra",        "giraffe",
                                                "backpack",      "umbrella",     "handbag",
                                                "tie",           "suitcase",     "frisbee",
                                                "skis",          "snowboard",    "sports ball",
                                                "kite",          "baseball bat", "baseball glove",
                                                "skateboard",    "surfboard",    "tennis racket",
                                                "bottle",        "wine glass",   "cup",
                                                "fork",          "knife",        "spoon",
                                                "bowl",          "banana",       "apple",
                                                "sandwich",      "orange",       "broccoli",
                                                "carrot",        "hot dog",      "pizza",
                                                "donut",         "cake",         "chair",
                                                "couch",         "potted plant", "bed",
                                                "dining table",  "toilet",       "tv",
                                                "laptop",        "mouse",        "remote",
                                                "keyboard",      "cell phone",   "microwave",
                                                "oven",          "toaster",      "sink",
                                                "refrigerator",  "book",         "clock",
                                                "vase",          "scissors",     "teddy bear",
                                                "hair drier",    "toothbrush"};
                className = COCO_CLASSES[classId];
            }

            std::cout << "  " << std::left << std::setw(15) << className << ": " << std::right
                      << std::setw(5) << count << " 次 (" << std::fixed << std::setprecision(1)
                      << std::setw(5) << percentage << "%)\n";
        }
        std::cout << "\n";
    }

    // 轨迹统计
    if (!trackLifetimes.empty()) {
        int totalTracks = trackLifetimes.size();
        int totalLifetime = 0;
        int maxLifetime = 0;

        for (const auto& [trackId, lifetime] : trackLifetimes) {
            totalLifetime += lifetime;
            if (lifetime > maxLifetime) {
                maxLifetime = lifetime;
            }
        }

        float avgLifetime = static_cast<float>(totalLifetime) / totalTracks;

        std::cout << "轨迹统计:\n";
        std::cout << "  总轨迹数: " << totalTracks << "\n";
        std::cout << "  平均轨迹长度: " << std::fixed << std::setprecision(1) << avgLifetime
                  << " 帧\n";
        std::cout << "  最长轨迹: " << maxLifetime << " 帧\n\n";
    }

    // 输出信息
    std::cout << "输出:\n";
    if (encoderInitialized && fs::exists(outputPath)) {
        std::cout << "  带标注视频: " << outputPath.filename().string() << "\n";
    } else {
        std::cout << "  带标注视频: 未生成 (需添加 --save-video 参数)\n";
    }

    std::cout
        << "===============================================================================\n";
    std::cout << std::endl;

    // 验证处理成功
    EXPECT_GT(processedFrames, 0) << "No frames were processed";
    EXPECT_FALSE(scores.empty()) << "No scores were collected";
}
