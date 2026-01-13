#include <gtest/gtest.h>

#include <algorithm>
#include <cstddef>
#include <filesystem>
#include <fstream>
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
#include "TestPathUtils.h"

namespace fs = std::filesystem;

class TextDetectorTest : public ::testing::Test {
protected:
    void SetUp() override {
#ifdef _WIN32
        // 设置控制台输出为 UTF-8，防止中文乱码
        SetConsoleOutputCP(CP_UTF8);
#endif

        // 使用 TestPathUtils 查找模型目录
        fs::path modelsDir = TestPathUtils::findModelFile("ch_PP-OCRv4_det_infer.onnx");
        if (modelsDir.empty()) {
            FAIL() << "OCR detection model not found. Current path: "
                   << TestPathUtils::pathToUtf8String(fs::current_path());
        }
        modelsDir = modelsDir.parent_path();

        fs::path detModelPath = modelsDir / "ch_PP-OCRv4_det_infer.onnx";
        fs::path recModelPath = modelsDir / "ch_PP-OCRv4_rec_infer.onnx";

        ASSERT_TRUE(fs::exists(detModelPath))
            << "Detection model not found at: "
            << TestPathUtils::pathToUtf8String(detModelPath);
        ASSERT_TRUE(fs::exists(recModelPath))
            << "Recognition model not found at: "
            << TestPathUtils::pathToUtf8String(recModelPath);

        std::cout << "[Setup] Using Det Model: "
                  << TestPathUtils::pathToUtf8String(detModelPath) << std::endl;
        std::cout << "[Setup] Using Rec Model: "
                  << TestPathUtils::pathToUtf8String(recModelPath) << std::endl;

        modelManager_ = &KeyFrame::ModelManager::GetInstance();

        // 加载检测模型
        if (!modelManager_->hasModel("ch_PP-OCRv4_det_infer.onnx")) {
            modelManager_->loadModel("ch_PP-OCRv4_det_infer.onnx", detModelPath.u8string(),
                                     KeyFrame::ModelManager::FrameWorkType::ONNXRuntime);
        }

        // 加载识别模型
        if (!modelManager_->hasModel("ch_PP-OCRv4_rec_infer.onnx")) {
            modelManager_->loadModel("ch_PP-OCRv4_rec_infer.onnx", recModelPath.u8string(),
                                     KeyFrame::ModelManager::FrameWorkType::ONNXRuntime);
        }

        detector_ = std::make_unique<KeyFrame::TextDetector>(*modelManager_);
    }

    KeyFrame::ModelManager* modelManager_;
    std::unique_ptr<KeyFrame::TextDetector> detector_;
};

TEST_F(TextDetectorTest, ImageComparisonTest) {
    // 使用 TestPathUtils 查找测试资源目录
    fs::path testImageDir = TestPathUtils::findAssetsDir("1-anytype.png");

    ASSERT_FALSE(testImageDir.empty()) << "Test images not found!";
    std::cout << "[Test] Using Assets Dir: "
              << TestPathUtils::pathToUtf8String(testImageDir) << std::endl;

    auto imreadUnicode = [](const fs::path& path) {
        std::ifstream ifs(path, std::ios::binary | std::ios::ate);
        if (!ifs.is_open())
            return cv::Mat();
        auto size = ifs.tellg();
        ifs.seekg(0, std::ios::beg);
        std::vector<char> buffer(size);
        if (!ifs.read(buffer.data(), size))
            return cv::Mat();
        return cv::imdecode(buffer, cv::IMREAD_COLOR);
    };

    cv::Mat img1 = imreadUnicode(testImageDir / "1-anytype.png");
    cv::Mat img2 = imreadUnicode(testImageDir / "2-code.png");
    cv::Mat img3 = imreadUnicode(testImageDir / "3-codeWithSmallChange.png");

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
