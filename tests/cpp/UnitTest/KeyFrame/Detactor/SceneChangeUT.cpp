#include <gtest/gtest.h>

#include <filesystem>
#include <iostream>
#include <memory>
#include <opencv2/core/mat.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/opencv.hpp>
#include <sstream>
#include <string>

#include "ModelManager.h"
#include "SceneChangeDetector.h"

namespace fs = std::filesystem;

class SceneChangeDetectorTest : public ::testing::Test {
protected:
    void SetUp() override {
        // 智能查找模型路径，支持多种运行场景
        fs::path exePath = fs::current_path();
        fs::path modelPath;

        // 尝试多个可能的路径
        std::vector<fs::path> possiblePaths = {
            // 1. 当前目录就是项目根目录（从项目根目录运行）
            exePath / "Models" / "MobileNet-v3-Small.onnx",
            // 2. 从 build/bin 目录运行，回退两级
            exePath.parent_path().parent_path() / "Models" / "MobileNet-v3-Small.onnx",
            // 3. 模型文件在 build/bin 目录下（已复制）
            exePath / "MobileNet-v3-Small.onnx",
            // 4. 使用 CMake 定义的宏（作为后备方案）
            fs::path(TEST_MODELS_DIR) / "MobileNet-v3-Small.onnx"};

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
            oss << "Model file not found. Tried paths:\n";
            for (size_t i = 0; i < possiblePaths.size(); ++i) {
                oss << "  " << (i + 1) << ". " << possiblePaths[i].string() << "\n";
            }
            oss << "Current path: " << fs::current_path().string();
            FAIL() << oss.str();
        }

        std::cout << "[Setup] Using model path: " << modelPath.string() << std::endl;

        // 初始化 ModelManager 并加载模型
        auto& mm = KeyFrame::ModelManager::GetInstance();
        if (!mm.hasModel("MobileNet-v3-Small")) {
            mm.loadModel("MobileNet-v3-Small", modelPath.string(),
                         KeyFrame::ModelManager::FrameWorkType::ONNXRuntime);
        }

        detector = std::make_unique<KeyFrame::SceneChangeDetector>(mm);
    }

    std::unique_ptr<KeyFrame::SceneChangeDetector> detector;
};

/**
 * @brief 测试场景变化检测
 * 使用三张图片进行流水线测试：
 * 1. 1-anytype.png (初始帧)
 * 2. 2-code.png (与 1 差异很大) -> 预期 isSceneChange = true, Score 高
 * 3. 3-codeWithSmallChange.png (与 2 差异很小) -> 预期 isSceneChange = false, Score 低
 */
TEST_F(SceneChangeDetectorTest, SceneChangeFlowTest) {
    // 智能查找测试资源目录，支持多种运行场景
    fs::path exePath = fs::current_path();
    fs::path assetsDir;

    std::vector<fs::path> possibleAssetDirs = {
        // 1. 当前目录就是项目根目录（从项目根目录运行）
        exePath / "tests" / "cpp" / "UnitTest" / "KeyFrame" / "TestImage",
        // 2. 从 build/bin 目录运行，回退两级
        exePath.parent_path().parent_path() / "tests" / "cpp" / "UnitTest" / "KeyFrame" /
            "TestImage",
        // 3. 使用 CMake 定义的宏（作为后备方案）
        fs::path(TEST_ASSETS_DIR)};

    bool found = false;
    for (const auto& dir : possibleAssetDirs) {
        if (fs::exists(dir)) {
            assetsDir = dir;
            found = true;
            break;
        }
    }

    ASSERT_TRUE(found) << "Test assets directory not found!";

    std::cout << "[Test] Using assets directory: " << assetsDir.string() << std::endl;

    cv::Mat img1 = cv::imread((assetsDir / "1-anytype.png").string());
    cv::Mat img2 = cv::imread((assetsDir / "2-code.png").string());
    cv::Mat img3 = cv::imread((assetsDir / "3-codeWithSmallChange.png").string());

    ASSERT_FALSE(img1.empty()) << "Failed to load 1-anytype.png from " << assetsDir;
    ASSERT_FALSE(img2.empty()) << "Failed to load 2-code.png from " << assetsDir;
    ASSERT_FALSE(img3.empty()) << "Failed to load 3-codeWithSmallChange.png from " << assetsDir;

    std::cout << "\n[Test Info] Starting Scene Change Detection Test..." << std::endl;

    // Step 1: Initial frame
    auto res1 = detector->detect(img1);
    std::cout << "[Step 1] Initial Frame (Image 1): "
              << "Score=" << res1.score << ", Similarity=" << res1.similarity
              << ", IsSceneChange=" << (res1.isSceneChange ? "YES" : "NO") << std::endl;
    EXPECT_FALSE(res1.isSceneChange);

    // Step 2: Compare Image 2 with Image 1 (Significant change)
    auto res2 = detector->detect(img2);
    std::cout << "[Step 2] Image 1 vs Image 2 (Huge Change): "
              << "Score=" << res2.score << ", Similarity=" << res2.similarity
              << ", IsSceneChange=" << (res1.isSceneChange ? "YES" : "NO") << " -> "
              << (res2.isSceneChange ? "YES" : "NO") << std::endl;

    // 我们预期 1 和 2 之间有显著变化
    EXPECT_TRUE(res2.isSceneChange);
    EXPECT_GT(res2.score, 0.4f);  // 归一化后的分数应该在较高区间

    // Step 3: Compare Image 3 with Image 2 (Small change)
    auto res3 = detector->detect(img3);
    std::cout << "[Step 3] Image 2 vs Image 3 (Small Change): "
              << "Score=" << res3.score << ", Similarity=" << res3.similarity
              << ", IsSceneChange=" << (res2.isSceneChange ? "YES" : "NO") << " -> "
              << (res3.isSceneChange ? "YES" : "NO") << std::endl;

    // 我们预期 2 和 3 之间变化较小
    EXPECT_FALSE(res3.isSceneChange);
    EXPECT_LT(res3.score, 0.3f);  // 归一化后的分数应该在较低区间
}
