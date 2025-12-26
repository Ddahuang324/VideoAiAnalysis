#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "TensorBuffer.h"
#include "onnxruntime_c_api.h"
#include "onnxruntime_cxx_api.h"

namespace KeyFrame {

class ONNXSession {
public:
    struct Config {
        int intraOpNumThreads;
        int interOpNumThreads;
        bool enableCUDA;
        int cudaDeviceId;
        GraphOptimizationLevel optimizationLevel;

        Config()
            : intraOpNumThreads(4),
              interOpNumThreads(2),
              enableCUDA(false),
              cudaDeviceId(0),
              optimizationLevel(GraphOptimizationLevel::ORT_ENABLE_ALL) {}
    };

    ONNXSession(Ort::Env& env, const std::string& modelPath, const Config& config);
    ~ONNXSession() = default;

    /**
     * @brief 执行推理，结果返回为 vector (旧接口)
     */
    std::vector<std::vector<float>> run(const std::vector<std::vector<float>>& inputs);

    /**
     * @brief 执行推理，结果写入 TensorBuffer (高性能接口)
     * @param inputs 输入数据
     * @param outputBuffer 用于存储输出数据的内存池
     * @return 返回指向 outputBuffer 中各输出张量起始位置的指针列表
     */
    std::vector<float*> run(const std::vector<std::vector<float>>& inputs,
                            TensorBuffer& outputBuffer);

    std::vector<int64_t> getInputShape(size_t index = 0) const;
    std::vector<int64_t> getOutputShape(size_t index = 0) const;
    void warmUp();
    std::string getName() const { return modelName_; }

private:
    void extractMetadata();
    std::string extractModelName(const std::string& path);
    std::vector<Ort::Value> createInputTensors(const std::vector<std::vector<float>>& inputs);
    std::vector<std::vector<float>> extractOutputs(std::vector<Ort::Value>& outputTensors);

    /**
     * @brief 计算形状对应的元素总量，并填充动态维度
     * @param shape 形状引用（会被修改以填充动态维度）
     * @param dynamicValue 用于替换动态维度 (-1) 的值
     * @return 元素总量
     */
    size_t computeShapeAndCount(std::vector<int64_t>& shape, int64_t dynamicValue);

    Ort::Env* env_;
    std::unique_ptr<Ort::Session> session_;
    Ort::SessionOptions sessionOptions_;
    Ort::MemoryInfo memoryInfo_;
    std::vector<std::string> inputNodeNames_;
    std::vector<std::string> outputNodeNames_;
    std::vector<std::vector<int64_t>> inputShapes_;
    std::vector<std::vector<int64_t>> outputShapes_;
    std::string modelName_;
    Config config_;
};

}  // namespace KeyFrame