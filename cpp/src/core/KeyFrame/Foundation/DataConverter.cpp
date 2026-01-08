#include "DataConverter.h"

#include <opencv2/core/hal/interface.h>

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <ios>
#include <opencv2/core.hpp>
#include <opencv2/core/base.hpp>
#include <opencv2/core/mat.hpp>
#include <opencv2/core/types.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include <stdexcept>
#include <string>
#include <vector>

namespace KeyFrame {

std::vector<float> DataConverter::matToTensor(

    const cv::Mat& image,

    const cv::Size& targetSize,

    bool normalize,

    const std::vector<float>& mean,

    const std::vector<float>& std

) {
    // 1. Resize

    cv::Mat resized;

    cv::resize(image, resized, targetSize);

    // 2. 转换为浮点型

    cv::Mat floatImage;

    resized.convertTo(floatImage, CV_32FC3);

    // 3. 归一化

    if (normalize) {
        floatImage /= 255.0f;  // 给矩阵所有的元素除以255.0f
    }

    // 4. 标准化

    if (mean.size() == 3 && std.size() == 3) {
        standardize(floatImage, mean, std);
    }

    // 5. HWC → NCHW

    const int H = floatImage.rows;

    const int W = floatImage.cols;

    const int C = floatImage.channels();

    std::vector<float> tensor(C * H * W);

    // 手动重排(性能优化)

    for (int h = 0; h < H; ++h) {
        const float* row = floatImage.ptr<float>(h);  // 获取行指针

        for (int w = 0; w < W; ++w) {
            for (int c = 0; c < C; ++c) {
                // HWC: row[w*C + c]

                // NCHW: tensor[c*H*W + h*W + w]

                tensor[c * H * W + h * W + w] = row[w * C + c];

                /* tensor 索引说明：
                c * H * W 当前通道下颜色的分区
                h * W 当前行的偏移
                w 当前列的偏移
                */

                /*row 索引说明：
                w * C 当前列的偏移
                c 当前通道的偏移
                */
            }
        }
    }

    return tensor;
}

std::vector<float> DataConverter::hwcToNchw(

    const std::vector<float>& hwcData,

    int H, int W, int C

) {
    std::vector<float> nchwData(C * H * W);

    for (int h = 0; h < H; ++h) {
        for (int w = 0; w < W; ++w) {
            for (int c = 0; c < C; ++c) {
                int hwcIndex = h * W * C + w * C + c;

                int nchwIndex = c * H * W + h * W + w;

                nchwData[nchwIndex] = hwcData[hwcIndex];
            }
        }
    }

    return nchwData;
}

cv::Mat DataConverter::preprocessImage(

    const cv::Mat& image,

    const cv::Size& targetSize,

    bool normalize

) {
    cv::Mat resized;

    cv::resize(image, resized, targetSize);

    cv::Mat floatImage;

    resized.convertTo(floatImage, CV_32FC3);

    if (normalize) {
        floatImage /= 255.0f;
    }

    return floatImage;
}

void DataConverter::standardize(cv::Mat& image, const std::vector<float>& mean,
                                const std::vector<float>& std) {
    if (mean.size() != 3 || std.size() != 3) {
        throw std::invalid_argument("mean and std must have 3 elements");
    }

    // 分离通道

    std::vector<cv::Mat> channels(3);

    cv::split(image, channels);

    // 标准化每个通道

    for (int c = 0; c < 3; ++c) {
        channels[c] = (channels[c] - mean[c]) / std[c];
    }

    // 合并通道

    cv::merge(channels, image);
}

// ==================== Letterbox 函数实现 ====================

cv::Mat DataConverter::letterboxResize(const cv::Mat& image, const cv::Size& targetSize,
                                       LetterboxInfo& info, const cv::Scalar& fillColor) {
    if (image.empty()) {
        return cv::Mat();
    }

    // 记录原始尺寸
    info.origSize = image.size();

    // 计算缩放比例(保持宽高比)
    float scaleW = static_cast<float>(targetSize.width) / image.cols;
    float scaleH = static_cast<float>(targetSize.height) / image.rows;
    info.scale = std::min(scaleW, scaleH);

    // 计算缩放后的尺寸
    int newWidth = static_cast<int>(image.cols * info.scale);
    int newHeight = static_cast<int>(image.rows * info.scale);
    info.newSize = cv::Size(newWidth, newHeight);

    // 缩放图像
    cv::Mat resized;
    cv::resize(image, resized, info.newSize, 0, 0, cv::INTER_LINEAR);

    // 计算填充量(居中对齐)
    info.padLeft = (targetSize.width - newWidth) / 2;
    info.padTop = (targetSize.height - newHeight) / 2;
    int padRight = targetSize.width - newWidth - info.padLeft;
    int padBottom = targetSize.height - newHeight - info.padTop;

    // 创建填充后的图像
    cv::Mat letterboxed;
    cv::copyMakeBorder(resized, letterboxed, info.padTop, padBottom, info.padLeft, padRight,
                       cv::BORDER_CONSTANT, fillColor);

    return letterboxed;
}

std::vector<float> DataConverter::matToTensorLetterbox(const cv::Mat& image,
                                                       const cv::Size& targetSize,
                                                       LetterboxInfo& info, bool normalize,
                                                       const std::vector<float>& mean,
                                                       const std::vector<float>& std) {
    // 1. Letterbox 缩放
    cv::Mat letterboxed = letterboxResize(image, targetSize, info);

    if (letterboxed.empty()) {
        return {};
    }

    // 2. 转换为浮点型
    cv::Mat floatImage;
    letterboxed.convertTo(floatImage, CV_32FC3);

    // 3. 归一化
    if (normalize) {
        floatImage /= 255.0f;
    }

    // 4. 标准化
    if (mean.size() == 3 && std.size() == 3) {
        standardize(floatImage, mean, std);
    }

    // 5. HWC → CHW (不是 NCHW,因为调用方会再转换)
    const int H = floatImage.rows;
    const int W = floatImage.cols;
    const int C = floatImage.channels();

    std::vector<float> tensor(C * H * W);

    // 手动重排
    for (int h = 0; h < H; ++h) {
        const float* row = floatImage.ptr<float>(h);
        for (int w = 0; w < W; ++w) {
            for (int c = 0; c < C; ++c) {
                tensor[c * H * W + h * W + w] = row[w * C + c];
            }
        }
    }

    return tensor;
}

cv::Mat DataConverter::readImage(const std::string& utf8Path, int flags) {
    try {
        // 使用 std::filesystem 处理 UTF-8 路径 (C++17)
#ifdef _WIN32
        std::filesystem::path p = std::filesystem::u8path(utf8Path);
#else
        std::filesystem::path p(utf8Path);
#endif

        if (!std::filesystem::exists(p)) {
            return cv::Mat();
        }

        // 以二进制模式读取文件到内存
        std::ifstream file(p, std::ios::binary | std::ios::ate);
        if (!file.is_open()) {
            return cv::Mat();
        }

        std::streamsize size = file.tellg();
        file.seekg(0, std::ios::beg);

        std::vector<char> buffer(size);
        if (!file.read(buffer.data(), size)) {
            return cv::Mat();
        }

        // 从内存解码
        return cv::imdecode(buffer, flags);
    } catch (...) {
        return cv::Mat();
    }
}

bool DataConverter::writeImage(const std::string& utf8Path, const cv::Mat& image) {
    if (image.empty()) {
        return false;
    }

    try {
#ifdef _WIN32
        std::filesystem::path p = std::filesystem::u8path(utf8Path);
#else
        std::filesystem::path p(utf8Path);
#endif

        // 获取文件后缀以确定编码格式
        std::string ext = p.extension().string();
        if (ext.empty())
            ext = ".png";

        // 将图像编码到内存缓冲区
        std::vector<uchar> buffer;
        if (!cv::imencode(ext, image, buffer)) {
            return false;
        }

        // 写入文件
        std::ofstream file(p, std::ios::binary);
        if (!file.is_open()) {
            return false;
        }

        file.write(reinterpret_cast<const char*>(buffer.data()), buffer.size());
        return file.good();
    } catch (...) {
        return false;
    }
}

cv::Rect DataConverter::rescaleBox(const cv::Rect& box, const LetterboxInfo& info) {
    // 去除填充并缩放回原图
    int x = static_cast<int>((box.x - info.padLeft) / info.scale);
    int y = static_cast<int>((box.y - info.padTop) / info.scale);
    int w = static_cast<int>(box.width / info.scale);
    int h = static_cast<int>(box.height / info.scale);

    // 裁剪到原图范围内
    x = std::max(0, std::min(x, info.origSize.width - 1));
    y = std::max(0, std::min(y, info.origSize.height - 1));
    w = std::max(1, std::min(w, info.origSize.width - x));
    h = std::max(1, std::min(h, info.origSize.height - y));

    return cv::Rect(x, y, w, h);
}

}  // namespace KeyFrame
