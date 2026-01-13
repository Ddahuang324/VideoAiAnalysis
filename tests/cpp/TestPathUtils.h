#pragma once

#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

#ifdef _WIN32
#include <Windows.h>
#endif

namespace fs = std::filesystem;

/**
 * @brief 测试路径工具类
 *
 * 提供统一的路径查找和处理函数，解决中文路径编码问题
 * 支持多种运行场景（IDE直接运行、build目录运行等）
 */
namespace TestPathUtils {

/**
 * @brief 智能查找模型文件
 *
 * 按优先级顺序查找模型文件：
 * 1. CMake 定义的 TEST_MODELS_DIR 宏路径
 * 2. 当前目录下的 Models/
 * 3. build/bin/../Models/
 * 4. build/bin/ 目录
 *
 * @param modelName 模型文件名（如 "yolov8n.onnx"）
 * @return 找到的模型完整路径，如果未找到返回空路径
 */
inline fs::path findModelFile(const std::string& modelName) {
    fs::path exePath = fs::current_path();

    // 优先级顺序的路径列表
    std::vector<fs::path> possiblePaths = {
#ifdef TEST_MODELS_DIR
        // 1. CMake 定义的宏路径（最高优先级）
        fs::u8path(TEST_MODELS_DIR) / modelName,
#endif
        // 2. 当前目录下的 Models/
        exePath / "Models" / modelName,
        // 3. 从 build/bin 目录运行，回退两级到项目根目录
        exePath.parent_path().parent_path() / "Models" / modelName,
        // 4. 模型文件直接在 build/bin 目录下
        exePath / modelName,
    };

    for (const auto& path : possiblePaths) {
        if (fs::exists(path)) {
            return path;
        }
    }

    return {}; // 未找到
}

/**
 * @brief 智能查找测试资源目录
 *
 * 按优先级顺序查找测试资源目录：
 * 1. CMake 定义的 TEST_ASSETS_DIR 宏路径
 * 2. 当前目录下的 tests/cpp/UnitTest/KeyFrame/TestImage/
 * 3. build/bin/../tests/cpp/UnitTest/KeyFrame/TestImage/
 *
 * @param testFileName 测试文件名（用于验证目录正确性，如 "1-anytype.png"）
 * @return 找到的资源目录路径，如果未找到返回空路径
 */
inline fs::path findAssetsDir(const std::string& testFileName = "1-anytype.png") {
    fs::path exePath = fs::current_path();

    std::vector<fs::path> possibleDirs = {
#ifdef TEST_ASSETS_DIR
        // 1. CMake 定义的宏路径（最高优先级）
        fs::u8path(TEST_ASSETS_DIR),
#endif
        // 2. 当前目录下的测试资源路径
        exePath / "tests" / "cpp" / "UnitTest" / "KeyFrame" / "TestImage",
        // 3. 从 build/bin 目录运行，回退两级
        exePath.parent_path().parent_path() / "tests" / "cpp" / "UnitTest" / "KeyFrame" /
            "TestImage",
    };

    for (const auto& dir : possibleDirs) {
        if (fs::exists(dir / testFileName)) {
            return dir;
        }
    }

    return {}; // 未找到
}

/**
 * @brief 智能查找测试视频文件
 *
 * @param videoName 视频文件名（如 "recording_20251228_211312.mp4"）
 * @return 找到的视频完整路径，如果未找到返回空路径
 */
inline fs::path findTestVideo(const std::string& videoName) {
    fs::path exePath = fs::current_path();

    std::vector<fs::path> possiblePaths = {
        exePath / "tests" / "cpp" / "UnitTest" / "TestVideo" / videoName,
        exePath.parent_path().parent_path() / "tests" / "cpp" / "UnitTest" / "TestVideo" /
            videoName,
        exePath / videoName,
    };

    for (const auto& path : possiblePaths) {
        if (fs::exists(path)) {
            return path;
        }
    }

    return {}; // 未找到
}

/**
 * @brief 转换路径为 UTF-8 字符串（用于跨平台日志输出）
 *
 * @param path 文件系统路径
 * @return UTF-8 编码的字符串
 */
inline std::string pathToUtf8String(const fs::path& path) {
#ifdef _WIN32
    return path.u8string();
#else
    return path.string();
#endif
}

/**
 * @brief 打印路径查找失败信息
 *
 * @param resourceType 资源类型（如 "Model"、"Test Asset"）
 * @param resourceName 资源名称
 * @param possiblePaths 尝试过的路径列表
 */
inline void printPathNotFound(const std::string& resourceType, const std::string& resourceName,
                              const std::vector<fs::path>& possiblePaths) {
    std::cerr << "[错误] 未找到 " << resourceType << ": " << resourceName << std::endl;
    std::cerr << "[错误] 尝试过的路径：" << std::endl;
    for (size_t i = 0; i < possiblePaths.size(); ++i) {
        std::cerr << "  " << (i + 1) << ". " << pathToUtf8String(possiblePaths[i]) << std::endl;
    }
    std::cerr << "[错误] 当前工作目录: " << pathToUtf8String(fs::current_path()) << std::endl;
}

} // namespace TestPathUtils
