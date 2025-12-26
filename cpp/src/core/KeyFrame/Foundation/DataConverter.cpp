#include "DataConverter.h"

#include <opencv2/core/hal/interface.h>

#include <opencv2/core.hpp>
#include <opencv2/core/mat.hpp>
#include <opencv2/core/types.hpp>
#include <opencv2/imgproc.hpp>
#include <stdexcept>
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

}  // namespace KeyFrame
