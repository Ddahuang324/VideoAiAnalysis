#pragma once

#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <opencv2/core/mat.hpp>
#include <string>

namespace KeyFrame {

/**
 * @brief: 帧资源管理器，用于缓存预处理后的数据（如不同尺寸的Tensor），避免重复计算
 */
class FrameResource {
public:
    explicit FrameResource(const cv::Mat& frame) : originalFrame_(frame) {}

    // 获取原始帧
    const cv::Mat& getOriginalFrame() const { return originalFrame_; }

    /**
     * @brief: 获取指定尺寸和格式的预处理数据
     * @param key: 资源的唯一标识（例如 "scene_input_224x224"）
     * @param generator: 如果缓存不存在，用于生成数据的回调函数
     * @return: 预处理后的数据（通常是 cv::Mat 或 std::vector<float>）
     */
    template <typename T>
    std::shared_ptr<T> getOrGenerate(const std::string& key,
                                     std::function<std::shared_ptr<T>()> generator) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = cache_.find(key);
        if (it != cache_.end()) {
            return std::static_pointer_cast<T>(it->second);
        }

        auto data = generator();
        cache_[key] = data;
        return data;
    }

private:
    cv::Mat originalFrame_;
    std::map<std::string, std::shared_ptr<void>> cache_;
    std::mutex mutex_;
};

}  // namespace KeyFrame
