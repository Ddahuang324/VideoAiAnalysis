#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "TensorBuffer.h"
#include "core/Config/UnifiedConfig.h"
#include "onnxruntime_cxx_api.h"

namespace KeyFrame {

// 使用统一配置系统的类型别名
using ONNXSessionConfig = Config::ONNXSessionConfig;

class ONNXSession {
public:
    // 使用统一配置
    using Config = ONNXSessionConfig;

    // 输出信息结构体
    struct OutputInfo {
        float* data;          // 指向 TensorBuffer 中的数据
        size_t elementCount;  // 实际元素数量（已处理动态 batch）
    };

    ONNXSession(Ort::Env& env, const std::string& modelPath, const Config& config);
    ~ONNXSession() = default;

    std::vector<std::vector<float>> run(const std::vector<std::vector<float>>& inputs);

    std::vector<OutputInfo> run(const std::vector<std::vector<float>>& inputs,
                                TensorBuffer& outputBuffer);

    std::vector<OutputInfo> run(const std::vector<std::vector<float>>& inputs,
                                const std::vector<std::vector<int64_t>>& inputShapes,
                                TensorBuffer& outputBuffer);

    std::vector<int64_t> getInputShape(size_t index = 0) const;
    std::vector<int64_t> getOutputShape(size_t index = 0) const;
    void warmUp();
    std::string getName() const { return modelName_; }

private:
    void extractMetadata();
    std::string extractModelName(const std::string& path);
    std::vector<Ort::Value> createInputTensors(const std::vector<std::vector<float>>& inputs,
                                               std::vector<std::vector<int64_t>>& outShapes);
    std::vector<Ort::Value> createInputTensors(
        const std::vector<std::vector<float>>& inputs,
        const std::vector<std::vector<int64_t>>& explicitShapes);
    std::vector<std::vector<float>> extractOutputs(std::vector<Ort::Value>& outputTensors);

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
