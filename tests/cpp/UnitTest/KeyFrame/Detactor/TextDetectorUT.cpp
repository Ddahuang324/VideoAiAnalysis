#include <gtest/gtest.h>

#include <algorithm>
#include <cstddef>
#include <filesystem>
#include <iostream>
#include <memory>
#include <opencv2/core/mat.hpp>
#include <opencv2/core/types.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/opencv.hpp>
#include <string>
#include <vector>

#ifdef _WIN32
#    include <windows.h>
#    include <winnls.h>

#    include "consoleapi2.h"


#endif

#include "DataConverter.h"
#include "ModelManager.h"
#include "TextDetector.h"

namespace fs = std::filesystem;

class TextDetectorTest : public ::testing::Test {
protected:
    void SetUp() override {
#ifdef _WIN32
        // 设置控制台输出为 UTF-8，防止中文乱码
        SetConsoleOutputCP(CP_UTF8);
#endif

        // 尝试多个可能的路径查找模型
        fs::path exePath = fs::current_path();
        std::vector<fs::path> possibleModelsDirs = {
            fs::path(TEST_MODELS_DIR), exePath / "Models",
            exePath.parent_path().parent_path() / "Models",
            fs::path("D:/编程/项目/AiVideoAnalsysSystem/Models")};

        fs::path detModelPath;
        fs::path recModelPath;

        for (const auto& dir : possibleModelsDirs) {
            if (fs::exists(dir / "ch_PP-OCRv4_det_infer.onnx")) {
                detModelPath = dir / "ch_PP-OCRv4_det_infer.onnx";
                recModelPath = dir / "ch_PP-OCRv4_rec_infer.onnx";
                break;
            }
        }

        ASSERT_FALSE(detModelPath.empty())
            << "Detection model not found. Checked paths include: " << TEST_MODELS_DIR;
        ASSERT_FALSE(recModelPath.empty()) << "Recognition model not found.";

        std::cout << "[Setup] Using Det Model: " << detModelPath.string() << std::endl;
        std::cout << "[Setup] Using Rec Model: " << recModelPath.string() << std::endl;

        modelManager_ = &KeyFrame::ModelManager::GetInstance();

        // 加载检测模型
        if (!modelManager_->hasModel("ch_PP-OCRv4_det_infer.onnx")) {
            modelManager_->loadModel("ch_PP-OCRv4_det_infer.onnx", detModelPath.string(),
                                     KeyFrame::ModelManager::FrameWorkType::ONNXRuntime);
        }

        // 加载识别模型
        if (!modelManager_->hasModel("ch_PP-OCRv4_rec_infer.onnx")) {
            modelManager_->loadModel("ch_PP-OCRv4_rec_infer.onnx", recModelPath.string(),
                                     KeyFrame::ModelManager::FrameWorkType::ONNXRuntime);
        }

        detector_ = std::make_unique<KeyFrame::TextDetector>(*modelManager_);
    }

    KeyFrame::ModelManager* modelManager_;
    std::unique_ptr<KeyFrame::TextDetector> detector_;
};

TEST_F(TextDetectorTest, ImageComparisonTest) {
    // 尝试多个可能的路径查找测试图片
    fs::path exePath = fs::current_path();
    std::vector<fs::path> possibleAssetsDirs = {
        fs::path(TEST_ASSETS_DIR), exePath / "tests/cpp/UnitTest/KeyFrame/TestImage",
        exePath.parent_path().parent_path() / "tests/cpp/UnitTest/KeyFrame/TestImage",
        fs::path("D:/编程/项目/AiVideoAnalsysSystem/tests/cpp/UnitTest/KeyFrame/TestImage")};

    fs::path testImageDir;
    for (const auto& dir : possibleAssetsDirs) {
        if (fs::exists(dir / "1-anytype.png")) {
            testImageDir = dir;
            break;
        }
    }

    ASSERT_FALSE(testImageDir.empty())
        << "Test images not found. Checked paths include: " << TEST_ASSETS_DIR;
    std::cout << "[Test] Using Assets Dir: " << testImageDir.string() << std::endl;

    cv::Mat img1 = cv::imread((testImageDir / "image1.png").string());
    cv::Mat img2 = cv::imread((testImageDir / "image2.png").string());
    cv::Mat img3 = cv::imread((testImageDir / "image3.png").string());

    ASSERT_FALSE(img1.empty()) << "Failed to load image1.png";
    ASSERT_FALSE(img2.empty()) << "Failed to load image2.png";
    ASSERT_FALSE(img3.empty()) << "Failed to load image3.png";

    // 统一图片尺寸，模拟视频流中的固定分辨率
    // 使用 Letterbox 保持宽高比缩放，避免拉伸导致文本变形
    cv::Size standardSize = img1.size();
    KeyFrame::DataConverter::LetterboxInfo info;

    if (img2.size() != standardSize) {
        img2 = KeyFrame::DataConverter::letterboxResize(img2, standardSize, info);
    }
    if (img3.size() != standardSize) {
        img3 = KeyFrame::DataConverter::letterboxResize(img3, standardSize, info);
    }

    std::cout << "Img1 size: " << img1.size() << std::endl;
    std::cout << "Img2 size: " << img2.size() << std::endl;
    std::cout << "Img3 size: " << img3.size() << std::endl;

    std::cout << "\n[Test] --- Processing Image 1 (anytype) ---" << std::endl;
    auto res1 = detector_->detect(img1);
    std::cout << "Score: " << res1.score << ", Coverage: " << res1.coverageRatio
              << ", Change: " << res1.changeRatio << std::endl;
    for (const auto& region : res1.textRegions) {
        std::cout << "  - Text: " << region.text << " [Conf: " << region.confidence << "]"
                  << std::endl;
    }

    std::cout << "\n[Test] --- Processing Image 2 (code) ---" << std::endl;
    auto res2 = detector_->detect(img2);
    std::cout << "Score: " << res2.score << ", Coverage: " << res2.coverageRatio
              << ", Change: " << res2.changeRatio << std::endl;
    for (size_t i = 0; i < (std::min)(res2.textRegions.size(), size_t(5)); ++i) {
        const auto& region = res2.textRegions[i];
        std::cout << "  - Rect: " << region.boundingBox << " Text: " << region.text << std::endl;
    }

    std::cout << "\n[Test] --- Processing Image 3 (codeWithSmallChange) ---" << std::endl;
    auto res3 = detector_->detect(img3);
    std::cout << "Score: " << res3.score << ", Coverage: " << res3.coverageRatio
              << ", Change: " << res3.changeRatio << std::endl;
    for (size_t i = 0; i < (std::min)(res3.textRegions.size(), size_t(5)); ++i) {
        const auto& region = res3.textRegions[i];
        std::cout << "  - Rect: " << region.boundingBox << " Text: " << region.text << std::endl;
    }

    // 验证逻辑
    // 1-2 变化大 (anytype vs code)
    std::cout << "\n[Verify] Comparing Image 1 and 2 (Expected large change)" << std::endl;
    EXPECT_GT(res2.changeRatio, 0.4f) << "Change ratio between 1 and 2 should be large";

    // 2-3 变化小 (code vs codeWithSmallChange)
    std::cout << "[Verify] Comparing Image 2 and 3 (Expected small change)" << std::endl;
    EXPECT_LT(res3.changeRatio, 0.3f) << "Change ratio between 2 and 3 should be small";

    // 2-3 文本应该有重复 (由于目前是占位符 [Text]，这里必然相等)
    if (!res2.textRegions.empty() && !res3.textRegions.empty()) {
        EXPECT_EQ(res2.textRegions[0].text, res3.textRegions[0].text);
    }
}
