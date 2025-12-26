#include "ONNXSession.h"

#include <cstddef>
#include <cstdint>
#include <exception>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

#include "Log.h"
#include "TensorBuffer.h"
#include "onnxruntime_c_api.h"
#include "onnxruntime_cxx_api.h"

namespace KeyFrame {

ONNXSession::ONNXSession(Ort::Env& env, const std::string& modelPath, const Config& config)
    : env_(&env),
      memoryInfo_(Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault)),
      modelName_(modelPath),
      config_(config) {
    Ort::SessionOptions sessionOptions;
    sessionOptions.SetIntraOpNumThreads(config.intraOpNumThreads);
    sessionOptions.SetGraphOptimizationLevel(config.optimizationLevel);
    sessionOptions.SetInterOpNumThreads(config.interOpNumThreads);

    if (config.enableCUDA) {
        OrtCUDAProviderOptions cudaOptions;
        cudaOptions.device_id = config.cudaDeviceId;
        sessionOptions.AppendExecutionProvider_CUDA(cudaOptions);
    }

    session_ = std::make_unique<Ort::Session>(*env_, std::filesystem::path(modelPath).c_str(),
                                              sessionOptions);

    extractMetadata();

    modelName_ = extractModelName(modelPath);
}

void ONNXSession::extractMetadata() {
    Ort::AllocatorWithDefaultOptions allocator;

    // Inputs
    size_t numInputs = session_->GetInputCount();
    for (size_t i = 0; i < numInputs; ++i) {
        auto name = session_->GetInputNameAllocated(i, allocator);
        inputNodeNames_.push_back(name.get());

        auto typeInfo = session_->GetInputTypeInfo(i);
        auto tensorInfo = typeInfo.GetTensorTypeAndShapeInfo();
        inputShapes_.push_back(tensorInfo.GetShape());
    }

    // Outputs
    size_t numOutputs = session_->GetOutputCount();
    for (size_t i = 0; i < numOutputs; ++i) {
        auto name = session_->GetOutputNameAllocated(i, allocator);
        outputNodeNames_.push_back(name.get());

        auto typeInfo = session_->GetOutputTypeInfo(i);
        auto tensorInfo = typeInfo.GetTensorTypeAndShapeInfo();
        outputShapes_.push_back(tensorInfo.GetShape());
    }
}

std::string ONNXSession::extractModelName(const std::string& path) {
    return std::filesystem::path(path).filename().string();
}

std::vector<std::vector<float>> ONNXSession::run(const std::vector<std::vector<float>>& inputs) {
    auto inputTensors = createInputTensors(inputs);

    std::vector<const char*> inputNames;
    for (const auto& name : inputNodeNames_) {
        inputNames.push_back(name.c_str());
    }

    std::vector<const char*> outputNames;
    for (const auto& name : outputNodeNames_) {
        outputNames.push_back(name.c_str());
    }

    auto outputTensors =
        session_->Run(Ort::RunOptions{nullptr}, inputNames.data(), inputTensors.data(),
                      inputTensors.size(), outputNames.data(), outputNames.size());

    return extractOutputs(outputTensors);
}

std::vector<float*> ONNXSession::run(const std::vector<std::vector<float>>& inputs,
                                     TensorBuffer& outputBuffer) {
    auto inputTensors = createInputTensors(inputs);

    // 从第一个输入推断当前的 Batch Size
    int64_t currentBatchSize = 1;
    if (!inputs.empty() && !inputShapes_.empty()) {
        std::vector<int64_t> shape = inputShapes_[0];
        size_t elementsPerBatch = computeShapeAndCount(shape, 1);
        if (elementsPerBatch > 0) {
            currentBatchSize = static_cast<int64_t>(inputs[0].size() / elementsPerBatch);
        }
    }

    std::vector<const char*> inputNames;
    for (const auto& name : inputNodeNames_) {
        inputNames.push_back(name.c_str());
    }

    std::vector<const char*> outputNames;
    for (const auto& name : outputNodeNames_) {
        outputNames.push_back(name.c_str());
    }

    // 准备输出 Tensor，直接使用 TensorBuffer 的内存，避免二次拷贝
    std::vector<Ort::Value> outputTensors;
    std::vector<float*> outputPtrs;
    std::vector<std::vector<int64_t>> actualOutputShapes;

    for (size_t i = 0; i < outputShapes_.size(); ++i) {
        std::vector<int64_t> shape = outputShapes_[i];
        size_t elementCount = computeShapeAndCount(shape, currentBatchSize);
        actualOutputShapes.push_back(shape);

        float* ptr = outputBuffer.allocate(elementCount);
        outputPtrs.push_back(ptr);

        outputTensors.push_back(Ort::Value::CreateTensor<float>(memoryInfo_, ptr, elementCount,
                                                                actualOutputShapes.back().data(),
                                                                actualOutputShapes.back().size()));
    }

    // 执行推理，结果将直接写入 outputBuffer
    session_->Run(Ort::RunOptions{nullptr}, inputNames.data(), inputTensors.data(),
                  inputTensors.size(), outputNames.data(), outputTensors.data(),
                  outputTensors.size());

    return outputPtrs;
}

std::vector<int64_t> ONNXSession::getInputShape(size_t index) const {
    if (index < inputShapes_.size())
        return inputShapes_[index];
    return {};
}

std::vector<int64_t> ONNXSession::getOutputShape(size_t index) const {
    if (index < outputShapes_.size())
        return outputShapes_[index];
    return {};
}

std::vector<Ort::Value> ONNXSession::createInputTensors(
    const std::vector<std::vector<float>>& inputs) {
    std::vector<Ort::Value> tensors;
    for (size_t i = 0; i < inputs.size(); ++i) {
        const auto& data = inputs[i];
        std::vector<int64_t> actualShape = inputShapes_[i];

        // 计算静态部分的元素乘积，并推断动态维度
        int64_t staticElements = 1;
        for (auto dim : actualShape) {
            if (dim > 0)
                staticElements *= dim;
        }

        int64_t dynamicValue =
            (staticElements > 0) ? static_cast<int64_t>(data.size() / staticElements) : 1;
        computeShapeAndCount(actualShape, dynamicValue);

        // 把对应的输入数据和格式转换成ONNX的tensor
        tensors.push_back(
            Ort::Value::CreateTensor<float>(memoryInfo_, const_cast<float*>(data.data()),
                                            data.size(), actualShape.data(), actualShape.size()));
    }
    return tensors;
}

std::vector<std::vector<float>> ONNXSession::extractOutputs(
    std::vector<Ort::Value>& outputTensors) {
    std::vector<std::vector<float>> outputs;
    for (auto& tensor : outputTensors) {
        float* floatData = tensor.GetTensorMutableData<float>();  // 获取tensor的指针
        auto tensorInfo = tensor.GetTensorTypeAndShapeInfo();     // 获取tensor的类型和形状信息
        size_t count = tensorInfo.GetElementCount();              // 数据的大小
        outputs.emplace_back(floatData, floatData + count);
    }
    return outputs;
}

void ONNXSession::warmUp() {
    if (inputShapes_.empty()) {
        LOG_INFO("[ONNXSession] No input shapes found, skipping warm up.");
        return;
    }

    try {
        std::vector<std::vector<float>> dummyInputs;
        for (const auto& shape : inputShapes_) {
            std::vector<int64_t> actualShape = shape;
            size_t elementCount = computeShapeAndCount(actualShape, 1);
            dummyInputs.emplace_back(elementCount, 0.0f);
        }

        run(dummyInputs);
        LOG_INFO("[ONNXSession] Model '" + modelName_ + "' warmed up successfully.");
    } catch (const std::exception& e) {
        LOG_ERROR("[ONNXSession] Warm up failed for model '" + modelName_ +
                  "': " + std::string(e.what()));
    }
}

size_t ONNXSession::computeShapeAndCount(std::vector<int64_t>& shape, int64_t dynamicValue) {
    size_t count = 1;
    for (auto& dim : shape) {
        if (dim <= 0) {
            dim = dynamicValue;
        }
        count *= static_cast<size_t>(dim);
    }
    return count;
}

}  // namespace KeyFrame
