#include <gtest/gtest.h>

#include <cstddef>
#include <filesystem>
#include <iostream>
#include <memory>
#include <opencv2/core/mat.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/opencv.hpp>
#include <string>
#include <vector>

#include "FrameResource.h"
#include "IFrameAnalyzer.h"
#include "ModelManager.h"
#include "MotionDetector.h"
#include "SceneChangeDetector.h"
#include "StandardFrameAnalyzer.h"
#include "TextDetector.h"

namespace fs = std::filesystem;

class FrameAnalyzerImageTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Setup paths
        fs::path exePath = fs::current_path();

        // 1. Locate Models
        std::vector<fs::path> possibleModelDirs = {
            fs::path(TEST_MODELS_DIR), exePath / "Models",
            exePath.parent_path().parent_path() / "Models",
            fs::path("D:/编程/项目/AiVideoAnalsysSystem/Models")};

        fs::path yoloPath, mobileNetPath, ocrDetPath, ocrRecPath;

        for (const auto& dir : possibleModelDirs) {
            if (fs::exists(dir / "yolov8n.onnx"))
                yoloPath = dir / "yolov8n.onnx";
            if (fs::exists(dir / "MobileNet-v3-Small.onnx"))
                mobileNetPath = dir / "MobileNet-v3-Small.onnx";
            if (fs::exists(dir / "ch_PP-OCRv4_det_infer.onnx"))
                ocrDetPath = dir / "ch_PP-OCRv4_det_infer.onnx";
            if (fs::exists(dir / "ch_PP-OCRv4_rec_infer.onnx"))
                ocrRecPath = dir / "ch_PP-OCRv4_rec_infer.onnx";
        }

        if (yoloPath.empty() || mobileNetPath.empty() || ocrDetPath.empty() || ocrRecPath.empty()) {
            std::cout << "[Warning] Some models not found, tests might fail or skip." << std::endl;
        }

        // 2. Initialize ModelManager
        auto& mm = KeyFrame::ModelManager::GetInstance();

        if (!yoloPath.empty() && !mm.hasModel("yolov8n.onnx"))
            mm.loadModel("yolov8n.onnx", yoloPath.string(),
                         KeyFrame::ModelManager::FrameWorkType::ONNXRuntime);

        if (!mobileNetPath.empty() && !mm.hasModel("MobileNet-v3-Small"))
            mm.loadModel("MobileNet-v3-Small", mobileNetPath.string(),
                         KeyFrame::ModelManager::FrameWorkType::ONNXRuntime);

        if (!ocrDetPath.empty() && !mm.hasModel("ch_PP-OCRv4_det_infer.onnx"))
            mm.loadModel("ch_PP-OCRv4_det_infer.onnx", ocrDetPath.string(),
                         KeyFrame::ModelManager::FrameWorkType::ONNXRuntime);

        if (!ocrRecPath.empty() && !mm.hasModel("ch_PP-OCRv4_rec_infer.onnx"))
            mm.loadModel("ch_PP-OCRv4_rec_infer.onnx", ocrRecPath.string(),
                         KeyFrame::ModelManager::FrameWorkType::ONNXRuntime);

        // 3. Create Detectors
        auto sceneDetector = std::make_shared<KeyFrame::SceneChangeDetector>(mm);

        KeyFrame::MotionDetector::Config motionConfig;  // Use default
        auto motionDetector =
            std::make_shared<KeyFrame::MotionDetector>(mm, motionConfig, "yolov8n.onnx");

        auto textDetector = std::make_shared<KeyFrame::TextDetector>(mm);

        // 4. Create Analyzer
        analyzer = std::make_unique<KeyFrame::StandardFrameAnalyzer>(sceneDetector, motionDetector,
                                                                     textDetector);
    }

    std::unique_ptr<KeyFrame::StandardFrameAnalyzer> analyzer;
};

TEST_F(FrameAnalyzerImageTest, AnalyzeImages1To6) {
    // 1. Locate Assets
    fs::path exePath = fs::current_path();
    std::vector<fs::path> possibleAssetDirs = {
        fs::path(TEST_ASSETS_DIR), exePath / "tests/cpp/UnitTest/KeyFrame/TestImage",
        exePath.parent_path().parent_path() / "tests/cpp/UnitTest/KeyFrame/TestImage",
        fs::path("D:/编程/项目/AiVideoAnalsysSystem/tests/cpp/UnitTest/KeyFrame/TestImage")};

    fs::path assetsDir;
    for (const auto& dir : possibleAssetDirs) {
        if (fs::exists(dir / "1-anytype.png")) {
            assetsDir = dir;
            break;
        }
    }

    ASSERT_FALSE(assetsDir.empty()) << "Test images directory not found.";
    std::cout << "[Test] Using assets directory: " << assetsDir.string() << std::endl;

    struct TestImage {
        std::string filename;
        std::string description;
        bool expectedLowScore;  // Expectation for the validation point (Images 3 and 6)
    };

    std::vector<TestImage> images = {
        {"1-anytype.png", "Initial Base Frame", false},
        {"2-code.png", "Large Change (High Score Expected)", false},
        {"3-codeWithSmallChange.png", "Small Change vs 2 (Low Score Expected)", true},
        {"4.png", "Sequence 4", false},
        {"5.png", "Sequence 5", false},
        {"6.png", "Sequence 6 (Redundant, Low Score Expected)", true}};

    struct ResultData {
        float sceneScore;
        bool isSceneChange;
    };
    std::vector<ResultData> results;

    for (size_t i = 0; i < images.size(); ++i) {
        std::string fullPath = (assetsDir / images[i].filename).string();
        cv::Mat img = cv::imread(fullPath);
        ASSERT_FALSE(img.empty()) << "Failed to load " << images[i].filename;

        // Create resource using constructor that accepts the frame
        auto resource = std::make_shared<KeyFrame::FrameResource>(img);

        KeyFrame::AnalysisContext context;
        context.frameIndex = i;
        context.timestamp = i * 1.0;  // Simulated 1fps

        auto scores = analyzer->analyzeFrame(resource, context);

        results.push_back({scores.sceneScore, scores.sceneChangeResult.isSceneChange});

        std::cout << "\n[Analyzed " << images[i].filename << " (" << images[i].description << ")]"
                  << std::endl;
        std::cout << "  Scene Score: " << scores.sceneScore << std::endl;
        std::cout << "  Is Scene Change: "
                  << (scores.sceneChangeResult.isSceneChange ? "YES" : "NO") << std::endl;
        std::cout << "  Similarity: " << scores.sceneChangeResult.similarity << std::endl;
    }

    // Validation Logic

    // 1. Verify Image 2 vs Image 3
    // Image 2 should have been a scene change (vs 1) OR have a relatively higher score if we
    // consider the transition. Image 3 should be a LOW score because it's only a small change
    // from 2. Note: The analyzer compares current frame vs PREVIOUS frame.

    // Check results[2] (Image 3)
    // We expect it NOT to be a scene change, and score to be low.
    EXPECT_FALSE(results[2].isSceneChange)
        << "Image 3 should NOT be a scene change (small difference from Image 2)";
    EXPECT_LT(results[2].sceneScore, 0.4f) << "Image 3 should have a low scene score";

    // 2. Verify Image 6
    // Assuming 6 is redundant to 5.
    EXPECT_FALSE(results[5].isSceneChange) << "Image 6 should NOT be a scene change (redundant)";
    EXPECT_LT(results[5].sceneScore, 0.4f) << "Image 6 should have a low scene score";

    std::cout
        << "\n[Success] Validated that Image 3 and Image 6 have low scores/redundancy as expected."
        << std::endl;
}
