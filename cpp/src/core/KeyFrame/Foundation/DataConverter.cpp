#include "DataConverter.h"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <ios>
#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include <stdexcept>
#include <string>
#include <vector>

#include "opencv2/core/base.hpp"
#include "opencv2/core/hal/interface.h"
#include "opencv2/core/mat.hpp"
#include "opencv2/core/types.hpp"


namespace KeyFrame {

// ========== Tensor Conversion ==========

std::vector<float> DataConverter::matToTensor(const cv::Mat& image, const cv::Size& targetSize,
                                              bool normalize, const std::vector<float>& mean,
                                              const std::vector<float>& std) {
    if (image.empty()) {
        return {};
    }

    // Resize and convert to float
    cv::Mat resized;
    cv::resize(image, resized, targetSize);

    // Ensure 3 channels (convert grayscale to BGR if needed)
    cv::Mat rgb;
    if (resized.channels() == 1) {
        cv::cvtColor(resized, rgb, cv::COLOR_GRAY2BGR);
    } else if (resized.channels() == 4) {
        cv::cvtColor(resized, rgb, cv::COLOR_BGRA2BGR);
    } else {
        rgb = resized;
    }

    cv::Mat floatImage;
    rgb.convertTo(floatImage, CV_32FC3);

    // Normalize to [0, 1]
    if (normalize) {
        floatImage /= 255.0f;
    }

    // Apply standardization
    if (mean.size() == 3 && std.size() == 3) {
        standardize(floatImage, mean, std);
    }

    // Convert HWC to CHW format
    const int H = floatImage.rows;
    const int W = floatImage.cols;
    const int C = floatImage.channels();

    std::vector<float> tensor(C * H * W);

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

std::vector<float> DataConverter::hwcToNchw(const std::vector<float>& hwcData, int H, int W,
                                            int C) {
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

cv::Mat DataConverter::preprocessImage(const cv::Mat& image, const cv::Size& targetSize,
                                       bool normalize) {
    cv::Mat resized;
    cv::resize(image, resized, targetSize);

    // Ensure 3 channels before float conversion
    cv::Mat rgb;
    if (resized.channels() == 1) {
        cv::cvtColor(resized, rgb, cv::COLOR_GRAY2BGR);
    } else if (resized.channels() == 4) {
        cv::cvtColor(resized, rgb, cv::COLOR_BGRA2BGR);
    } else {
        rgb = resized;
    }

    cv::Mat floatImage;
    rgb.convertTo(floatImage, CV_32FC3);

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

    std::vector<cv::Mat> channels(3);
    cv::split(image, channels);

    for (int c = 0; c < 3; ++c) {
        channels[c] = (channels[c] - mean[c]) / std[c];
    }

    cv::merge(channels, image);
}

// ========== Letterbox Functions ==========

cv::Mat DataConverter::letterboxResize(const cv::Mat& image, const cv::Size& targetSize,
                                       LetterboxInfo& info, const cv::Scalar& fillColor) {
    if (image.empty()) {
        return cv::Mat();
    }

    info.origSize = image.size();

    // Calculate scale while maintaining aspect ratio
    float scaleW = static_cast<float>(targetSize.width) / image.cols;
    float scaleH = static_cast<float>(targetSize.height) / image.rows;
    info.scale = std::min(scaleW, scaleH);

    // Calculate resized dimensions
    int newWidth = static_cast<int>(image.cols * info.scale);
    int newHeight = static_cast<int>(image.rows * info.scale);
    info.newSize = cv::Size(newWidth, newHeight);

    // Resize image
    cv::Mat resized;
    cv::resize(image, resized, info.newSize, 0, 0, cv::INTER_LINEAR);

    // Calculate padding (center alignment)
    info.padLeft = (targetSize.width - newWidth) / 2;
    info.padTop = (targetSize.height - newHeight) / 2;
    int padRight = targetSize.width - newWidth - info.padLeft;
    int padBottom = targetSize.height - newHeight - info.padTop;

    // Create letterboxed image
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
    cv::Mat letterboxed = letterboxResize(image, targetSize, info);
    if (letterboxed.empty()) {
        return {};
    }

    // Ensure 3 channels (convert grayscale to BGR if needed)
    cv::Mat rgb;
    if (letterboxed.channels() == 1) {
        cv::cvtColor(letterboxed, rgb, cv::COLOR_GRAY2BGR);
    } else if (letterboxed.channels() == 4) {
        cv::cvtColor(letterboxed, rgb, cv::COLOR_BGRA2BGR);
    } else {
        rgb = letterboxed;
    }

    // Convert to float
    cv::Mat floatImage;
    rgb.convertTo(floatImage, CV_32FC3);

    // Normalize
    if (normalize) {
        floatImage /= 255.0f;
    }

    // Standardize
    if (mean.size() == 3 && std.size() == 3) {
        standardize(floatImage, mean, std);
    }

    // Convert HWC to CHW
    const int H = floatImage.rows;
    const int W = floatImage.cols;
    const int C = floatImage.channels();

    std::vector<float> tensor(C * H * W);

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

cv::Rect DataConverter::rescaleBox(const cv::Rect& box, const LetterboxInfo& info) {
    int x = static_cast<int>((box.x - info.padLeft) / info.scale);
    int y = static_cast<int>((box.y - info.padTop) / info.scale);
    int w = static_cast<int>(box.width / info.scale);
    int h = static_cast<int>(box.height / info.scale);

    // Clamp to original image bounds
    x = std::max(0, std::min(x, info.origSize.width - 1));
    y = std::max(0, std::min(y, info.origSize.height - 1));
    w = std::max(1, std::min(w, info.origSize.width - x));
    h = std::max(1, std::min(h, info.origSize.height - y));

    return cv::Rect(x, y, w, h);
}

// ========== File I/O ==========

cv::Mat DataConverter::readImage(const std::string& utf8Path, int flags) {
    try {
#ifdef _WIN32
        std::filesystem::path p = std::filesystem::u8path(utf8Path);
#else
        std::filesystem::path p(utf8Path);
#endif

        if (!std::filesystem::exists(p)) {
            return cv::Mat();
        }

        // Read file into memory buffer
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

        std::string ext = p.extension().string();
        if (ext.empty()) {
            ext = ".png";
        }

        std::vector<uchar> buffer;
        if (!cv::imencode(ext, image, buffer)) {
            return false;
        }

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

}  // namespace KeyFrame
