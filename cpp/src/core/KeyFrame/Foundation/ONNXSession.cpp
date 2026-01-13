#include "ONNXSession.h"

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

#ifdef _WIN32
#    include <Windows.h>
#endif

#include "Log.h"
#include "TensorBuffer.h"
#include "onnxruntime_c_api.h"
#include "onnxruntime_cxx_api.h"

namespace KeyFrame {

// ========== UTF-8 Path Conversion (Windows) ==========
#ifdef _WIN32
static std::wstring utf8_to_wstring(const std::string& utf8_str) {
    if (utf8_str.empty()) {
        return std::wstring();
    }

    int size = MultiByteToWideChar(CP_UTF8, 0, utf8_str.c_str(), -1, nullptr, 0);
    if (size <= 0) {
        return std::wstring();
    }

    std::wstring wstr(size - 1, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, utf8_str.c_str(), -1, &wstr[0], size);

    return wstr;
}
#endif

// ========== Constructor ==========

ONNXSession::ONNXSession(Ort::Env& env, const std::string& modelPath, const Config& config)
    : env_(&env),
      memoryInfo_(Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault)),
      modelName_(modelPath),
      config_(config) {
    Ort::SessionOptions sessionOptions;
    sessionOptions.SetIntraOpNumThreads(config.intraOpNumThreads);
    sessionOptions.SetGraphOptimizationLevel(
        static_cast<GraphOptimizationLevel>(config.optimizationLevel));
    sessionOptions.SetInterOpNumThreads(config.interOpNumThreads);

    if (config.enableCUDA) {
        OrtCUDAProviderOptions cudaOptions;
        cudaOptions.device_id = config.cudaDeviceId;
        sessionOptions.AppendExecutionProvider_CUDA(cudaOptions);
    }

#ifdef _WIN32
    std::wstring widePath = utf8_to_wstring(modelPath);
    session_ = std::make_unique<Ort::Session>(*env_, widePath.c_str(), sessionOptions);
#else
    session_ = std::make_unique<Ort::Session>(*env_, modelPath.c_str(), sessionOptions);
#endif

    extractMetadata();
    modelName_ = extractModelName(modelPath);
}

// ========== Metadata Extraction ==========

void ONNXSession::extractMetadata() {
    Ort::AllocatorWithDefaultOptions allocator;

    // Extract input metadata
    size_t numInputs = session_->GetInputCount();
    for (size_t i = 0; i < numInputs; ++i) {
        auto name = session_->GetInputNameAllocated(i, allocator);
        inputNodeNames_.push_back(name.get());

        auto typeInfo = session_->GetInputTypeInfo(i);
        auto tensorInfo = typeInfo.GetTensorTypeAndShapeInfo();
        inputShapes_.push_back(tensorInfo.GetShape());
    }

    // Extract output metadata
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
#ifdef _WIN32
    return std::filesystem::path(path).filename().u8string();
#else
    return std::filesystem::path(path).filename().string();
#endif
}

// ========== Inference (No Buffer) ==========

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

// ========== Inference (With Buffer) ==========

std::vector<ONNXSession::OutputInfo> ONNXSession::run(const std::vector<std::vector<float>>& inputs,
                                                      TensorBuffer& outputBuffer) {
    auto inputTensors = createInputTensors(inputs);

    // Infer batch size from first input
    int64_t currentBatchSize = 1;
    if (!inputs.empty() && !inputShapes_.empty()) {
        std::vector<int64_t> shape = inputShapes_[0];
        size_t elementsPerBatch = computeShapeAndCount(shape, 1);
        if (elementsPerBatch > 0) {
            currentBatchSize = static_cast<int64_t>(inputs[0].size() / elementsPerBatch);
        }
    }

    // Prepare output name pointers
    std::vector<const char*> inputNames, outputNames;
    for (const auto& name : inputNodeNames_)
        inputNames.push_back(name.c_str());
    for (const auto& name : outputNodeNames_)
        outputNames.push_back(name.c_str());

    // Allocate output tensors directly in buffer
    std::vector<Ort::Value> outputTensors;
    std::vector<OutputInfo> outputInfos;
    std::vector<std::vector<int64_t>> actualOutputShapes;

    for (size_t i = 0; i < outputShapes_.size(); ++i) {
        std::vector<int64_t> shape = outputShapes_[i];
        size_t elementCount = computeShapeAndCount(shape, currentBatchSize);
        actualOutputShapes.push_back(shape);

        float* ptr = outputBuffer.allocate(elementCount);
        outputInfos.push_back({ptr, elementCount});

        outputTensors.push_back(Ort::Value::CreateTensor<float>(memoryInfo_, ptr, elementCount,
                                                                actualOutputShapes.back().data(),
                                                                actualOutputShapes.back().size()));
    }

    session_->Run(Ort::RunOptions{nullptr}, inputNames.data(), inputTensors.data(),
                  inputTensors.size(), outputNames.data(), outputTensors.data(),
                  outputTensors.size());

    return outputInfos;
}

std::vector<ONNXSession::OutputInfo> ONNXSession::run(
    const std::vector<std::vector<float>>& inputs,
    const std::vector<std::vector<int64_t>>& inputShapes, TensorBuffer& outputBuffer) {
    auto inputTensors = createInputTensors(inputs, inputShapes);

    // Infer batch size from first input shape
    int64_t currentBatchSize = 1;
    if (!inputShapes.empty() && !inputShapes[0].empty()) {
        currentBatchSize = inputShapes[0][0];
    }

    // Prepare name pointers
    std::vector<const char*> inputNames, outputNames;
    for (const auto& name : inputNodeNames_)
        inputNames.push_back(name.c_str());
    for (const auto& name : outputNodeNames_)
        outputNames.push_back(name.c_str());

    // Check if we can pre-allocate outputs
    bool canPreAllocate = true;
    for (const auto& shape : outputShapes_) {
        for (size_t j = 1; j < shape.size(); ++j) {
            if (shape[j] <= 0) {
                canPreAllocate = false;
                break;
            }
        }
        if (!canPreAllocate)
            break;
    }

    if (!canPreAllocate) {
        // Let ORT allocate for dynamic shapes
        auto outputTensors =
            session_->Run(Ort::RunOptions{nullptr}, inputNames.data(), inputTensors.data(),
                          inputTensors.size(), outputNames.data(), outputNames.size());

        std::vector<OutputInfo> outputInfos;
        for (auto& tensor : outputTensors) {
            auto info = tensor.GetTensorTypeAndShapeInfo();
            size_t elementCount = info.GetElementCount();

            float* ptr = outputBuffer.allocate(elementCount);
            float* tensorData = tensor.GetTensorMutableData<float>();

            std::copy(tensorData, tensorData + elementCount, ptr);
            outputInfos.push_back({ptr, elementCount});
        }
        return outputInfos;
    }

    // Pre-allocate outputs in buffer
    std::vector<Ort::Value> outputTensors;
    std::vector<OutputInfo> outputInfos;
    std::vector<std::vector<int64_t>> actualOutputShapes;

    for (size_t i = 0; i < outputShapes_.size(); ++i) {
        std::vector<int64_t> shape = outputShapes_[i];
        size_t elementCount = computeShapeAndCount(shape, currentBatchSize);
        actualOutputShapes.push_back(shape);

        float* ptr = outputBuffer.allocate(elementCount);
        outputInfos.push_back({ptr, elementCount});

        outputTensors.push_back(Ort::Value::CreateTensor<float>(memoryInfo_, ptr, elementCount,
                                                                actualOutputShapes.back().data(),
                                                                actualOutputShapes.back().size()));
    }

    session_->Run(Ort::RunOptions{nullptr}, inputNames.data(), inputTensors.data(),
                  inputTensors.size(), outputNames.data(), outputTensors.data(),
                  outputTensors.size());

    return outputInfos;
}

// ========== Shape Queries ==========

std::vector<int64_t> ONNXSession::getInputShape(size_t index) const {
    if (index < inputShapes_.size()) {
        return inputShapes_[index];
    }
    return {};
}

std::vector<int64_t> ONNXSession::getOutputShape(size_t index) const {
    if (index < outputShapes_.size()) {
        return outputShapes_[index];
    }
    return {};
}

// ========== Input Tensor Creation ==========

std::vector<Ort::Value> ONNXSession::createInputTensors(
    const std::vector<std::vector<float>>& inputs) {
    std::vector<Ort::Value> tensors;

    for (size_t i = 0; i < inputs.size(); ++i) {
        const auto& data = inputs[i];
        std::vector<int64_t> actualShape = inputShapes_[i];

        // Compute static elements and infer dynamic dimension
        int64_t staticElements = 1;
        for (auto dim : actualShape) {
            if (dim > 0)
                staticElements *= dim;
        }

        int64_t dynamicValue =
            (staticElements > 0) ? static_cast<int64_t>(data.size() / staticElements) : 1;
        computeShapeAndCount(actualShape, dynamicValue);

        tensors.push_back(
            Ort::Value::CreateTensor<float>(memoryInfo_, const_cast<float*>(data.data()),
                                            data.size(), actualShape.data(), actualShape.size()));
    }

    return tensors;
}

std::vector<Ort::Value> ONNXSession::createInputTensors(
    const std::vector<std::vector<float>>& inputs,
    const std::vector<std::vector<int64_t>>& explicitShapes) {
    std::vector<Ort::Value> tensors;

    for (size_t i = 0; i < inputs.size(); ++i) {
        const auto& data = inputs[i];
        const std::vector<int64_t>& actualShape = explicitShapes[i];

        tensors.push_back(Ort::Value::CreateTensor<float>(
            memoryInfo_, const_cast<float*>(data.data()), data.size(),
            const_cast<int64_t*>(actualShape.data()), actualShape.size()));
    }

    return tensors;
}

// ========== Output Extraction ==========

std::vector<std::vector<float>> ONNXSession::extractOutputs(
    std::vector<Ort::Value>& outputTensors) {
    std::vector<std::vector<float>> outputs;

    for (auto& tensor : outputTensors) {
        float* floatData = tensor.GetTensorMutableData<float>();
        auto tensorInfo = tensor.GetTensorTypeAndShapeInfo();
        size_t count = tensorInfo.GetElementCount();
        outputs.emplace_back(floatData, floatData + count);
    }

    return outputs;
}

// ========== Warm Up ==========

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

// ========== Shape Computation ==========

size_t ONNXSession::computeShapeAndCount(std::vector<int64_t>& shape, int64_t dynamicValue) {
    size_t count = 1;
    int dynamicDimCount = 0;

    for (auto dim : shape) {
        if (dim <= 0)
            dynamicDimCount++;
    }

    if (dynamicDimCount > 1 && shape.size() == 4) {
        // Special handling for NCHW image input
        if (shape[0] <= 0) {
            shape[0] = 1;  // Assume batch size = 1
            dynamicDimCount--;
        }

        if (dynamicDimCount == 2) {
            // Two dynamic dimensions (usually H and W)
            int64_t remainingElements = dynamicValue;
            int64_t side = static_cast<int64_t>(std::sqrt(remainingElements));

            if (side * side == remainingElements) {
                for (auto& dim : shape) {
                    if (dim <= 0)
                        dim = side;
                }
            } else {
                // Allocate to last dimension
                for (size_t i = 0; i < shape.size(); ++i) {
                    if (shape[i] <= 0) {
                        shape[i] = remainingElements;
                        remainingElements = 1;
                    }
                }
            }
        } else {
            for (auto& dim : shape) {
                if (dim <= 0)
                    dim = dynamicValue;
            }
        }
    } else {
        for (auto& dim : shape) {
            if (dim <= 0)
                dim = dynamicValue;
        }
    }

    for (auto dim : shape) {
        count *= static_cast<size_t>(dim);
    }

    return count;
}

}  // namespace KeyFrame
