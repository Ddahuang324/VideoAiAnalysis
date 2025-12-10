#pragma once

#include <string>
#include <vector>

/**
 * @brief 视频处理器类
 *
 * 提供视频帧处理、分析等核心功能
 * 可以通过 pybind11 暴露给 Python 使用
 */
class VideoProcessor {
public:
    VideoProcessor();
    ~VideoProcessor();

    /**
     * @brief 初始化处理器
     * @return 是否初始化成功
     */
    bool initialize();

    /**
     * @brief 处理单帧视频
     * @param frame_data 帧数据
     * @return 处理结果
     */
    std::string processFrame(const std::string& frame_data) const;

    /**
     * @brief 批量处理多帧
     * @param frames 帧数据列表
     * @return 处理结果列表
     */
    std::vector<std::string> processFrames(const std::vector<std::string>& frames) const;

    /**
     * @brief 获取处理器信息
     * @return 信息字符串
     */
    std::string getInfo() const;

    /**
     * @brief 设置处理参数
     * @param key 参数名
     * @param value 参数值
     */
    void setParameter(const std::string& key, double value);

    /**
     * @brief 获取处理参数
     * @param key 参数名
     * @return 参数值
     */
    double getParameter(const std::string& key) const;

private:
    bool initialized_;
    std::string model_path_;
    double threshold_;
};
