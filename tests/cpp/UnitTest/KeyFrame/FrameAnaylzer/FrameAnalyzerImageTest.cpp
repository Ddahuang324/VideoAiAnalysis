#include <gtest/gtest.h>

#ifdef _WIN32
#    include <windows.h>
#endif

#include <cstddef>
#include <filesystem>
#include <iostream>
#include <memory>
#include <opencv2/core/mat.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/opencv.hpp>
#include <string>
#include <vector>

#include "DynamicCalculator.h"
#include "FrameResource.h"
#include "FrameScorer.h"
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
#ifdef _WIN32
        SetConsoleOutputCP(CP_UTF8);
#endif
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
            std::cout << "[警告] 未找到部分模型，测试可能会失败或被跳过。" << std::endl;
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

        // 5. Create Scorer
        KeyFrame::DynamicCalculator::Config dynamicConfig;
        auto dynamicCalculator = std::make_shared<KeyFrame::DynamicCalculator>(dynamicConfig);
        scorer = std::make_unique<KeyFrame::FrameScorer>(dynamicCalculator);
    }

    std::unique_ptr<KeyFrame::StandardFrameAnalyzer> analyzer;
    std::unique_ptr<KeyFrame::FrameScorer> scorer;
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

    ASSERT_FALSE(assetsDir.empty()) << "未找到测试图像目录。";
    std::cout << "[测试] 使用资源目录: " << assetsDir.string() << std::endl;

    struct TestImage {
        std::string filename;
        std::string description;
        bool expectedLowScore;  // 对验证点的预期（图 3 和图 6）
    };

    std::vector<TestImage> images = {
        {"1-anytype.png", "初始基础帧", false},
        {"2-code.png", "变化较大（预期高分）", false},
        {"3-codeWithSmallChange.png", "相对于图 2 变化较小（预期低分）", true},
        {"4.png", "序列 4", false},
        {"5.png", "序列 5", false},
        {"6.png", "序列 6（冗余，预期低分）", true}};

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

        auto rawScores = analyzer->analyzeFrame(resource, context);
        auto finalScore = scorer->score(rawScores, context);

        results.push_back({rawScores.sceneScore, rawScores.sceneChangeResult.isSceneChange});

        std::cout << "\n[分析完成 " << images[i].filename << " (" << images[i].description << ")]"
                  << std::endl;
        std::cout << "  最终得分: " << finalScore.finalscore << std::endl;
        std::cout << "  贡献度: 场景=" << finalScore.sceneContribution
                  << ", 运动=" << finalScore.motionContribution
                  << ", 文本=" << finalScore.textContribution << std::endl;
        std::cout << "  权重: 场景=" << finalScore.appliedWeights[0]
                  << ", 运动=" << finalScore.appliedWeights[1]
                  << ", 文本=" << finalScore.appliedWeights[2] << std::endl;
        std::cout << "  是否为场景切换: "
                  << (rawScores.sceneChangeResult.isSceneChange ? "是" : "否") << std::endl;
        std::cout << "  相似度: " << rawScores.sceneChangeResult.similarity << std::endl;
    }

    // 验证逻辑

    // 1. 验证图 2 vs 图 3
    // 图 2 应该是场景切换（对比图 1）或者具有较高的分数。
    // 图 3 应该得分较低，因为它相对于图 2 只有很小的变化。
    // 注意：分析器是将当前帧与上一帧进行对比。

    // 检查 results[2] (图 3)
    // 我们预期它不是场景切换，且得分较低。
    EXPECT_FALSE(results[2].isSceneChange) << "图 3 不应被视为场景切换（相对于图 2 差异较小）";
    EXPECT_LT(results[2].sceneScore, 0.4f) << "图 3 的场景得分应较低";

    // 2. 验证图 6
    // 假设图 6 相对于图 5 是冗余的。
    EXPECT_FALSE(results[5].isSceneChange) << "图 6 不应被视为场景切换（冗余帧）";
    EXPECT_LT(results[5].sceneScore, 0.4f) << "图 6 的场景得分应较低";

    std::cout << "\n[成功] 已验证图 3 和图 6 如预期具有低分/冗余。" << std::endl;
}
