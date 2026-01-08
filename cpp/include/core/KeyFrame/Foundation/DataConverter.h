#pragma once

#include <opencv2/core/mat.hpp>
#include <opencv2/core/types.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/opencv.hpp>
#include <string>
#include <vector>

namespace KeyFrame {

/**
 * @brief 数据转换工具类
 * 提供 OpenCV Mat 与深度学习 Tensor 之间的高效转换。
 */

class DataConverter {
public:
    /**
     * @brief Letterbox 变换信息
     * 用于记录图像预处理时的变换参数,供后处理时将检测框映射回原图
     */
    struct LetterboxInfo {
        float scale;        // 缩放比例
        int padTop;         // 上方填充像素数
        int padLeft;        // 左侧填充像素数
        cv::Size newSize;   // 缩放后的尺寸
        cv::Size origSize;  // 原始图像尺寸

        LetterboxInfo() : scale(1.0f), padTop(0), padLeft(0), newSize(0, 0), origSize(0, 0) {}
    };

    // ==================== 现有函数(保持不变) ====================

    static std::vector<float> matToTensor(

        const cv::Mat& image,

        const cv::Size& targetSize,

        bool normalize = true,

        const std::vector<float>& mean = {0.0f, 0.0f, 0.0f},

        const std::vector<float>& std = {1.0f, 1.0f, 1.0f}

    );

    static std::vector<float> hwcToNchw(

        const std::vector<float>& hwcData,

        int H, int W, int C

    );

    static cv::Mat preprocessImage(

        const cv::Mat& image,

        const cv::Size& targetSize,

        bool normalize = true

    );

    static void standardize(

        cv::Mat& image,

        const std::vector<float>& mean,

        const std::vector<float>& std

    );

    // ==================== 新增 Letterbox 函数 ====================

    /**
     * @brief Letterbox 缩放(保持宽高比)
     * @param image 输入图像
     * @param targetSize 目标尺寸(通常为正方形,如 640×640)
     * @param info 输出变换信息,用于后处理
     * @param fillColor 填充颜色(默认灰色 114)
     * @return 经过 Letterbox 处理的图像
     */
    static cv::Mat letterboxResize(const cv::Mat& image, const cv::Size& targetSize,
                                   LetterboxInfo& info,
                                   const cv::Scalar& fillColor = cv::Scalar(114, 114, 114));

    /**
     * @brief Letterbox + 归一化 + 转 Tensor(保持宽高比)
     * @param image 输入图像
     * @param targetSize 目标尺寸
     * @param info 输出变换信息
     * @param normalize 是否归一化到 [0,1]
     * @param mean 标准化均值
     * @param std 标准化标准差
     * @return HWC 格式的 Tensor 数据
     */
    static std::vector<float> matToTensorLetterbox(
        const cv::Mat& image, const cv::Size& targetSize, LetterboxInfo& info,
        bool normalize = true, const std::vector<float>& mean = {0.485f, 0.456f, 0.406f},
        const std::vector<float>& std = {0.229f, 0.224f, 0.225f});

    /**
     * @brief 从包含 UTF-8 字符的路径加载图像 (解决 Windows 下中文路径问题)
     * @param utf8Path UTF-8 编码的图像路径
     * @param flags OpenCV 读取标志 (默认 IMREAD_COLOR)
     * @return 加载的 cv::Mat, 失败则返回空容器
     */
    static cv::Mat readImage(const std::string& utf8Path, int flags = cv::IMREAD_COLOR);

    /**
     * @brief 将图像保存到包含 UTF-8 字符的路径 (解决 Windows 下中文路径问题)
     * @param utf8Path UTF-8 编码的目标路径
     * @param image 要保存的图像
     * @return 是否保存成功
     */
    static bool writeImage(const std::string& utf8Path, const cv::Mat& image);

    /**
     * @brief 将检测框从 Letterbox 空间映射回原图空间
     * @param box Letterbox 空间的检测框
     * @param info Letterbox 变换信息
     * @return 原图空间的检测框
     */
    static cv::Rect rescaleBox(const cv::Rect& box, const LetterboxInfo& info);
};

}  // namespace KeyFrame
