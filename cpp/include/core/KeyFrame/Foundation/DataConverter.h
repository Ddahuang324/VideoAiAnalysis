#pragma once

#include <opencv2/core/mat.hpp>
#include <opencv2/core/types.hpp>
#include <opencv2/opencv.hpp>
#include <vector>

namespace KeyFrame {

/**
 * @brief 数据转换工具类
 * 提供 OpenCV Mat 与深度学习 Tensor 之间的高效转换。
 */

class DataConverter {
public:
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
};

}  // namespace KeyFrame
