#include "ModelManager.h"

#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <utility>
#include <vector>

#include "Log.h"
#include "ONNXSession.h"
#include "onnxruntime_c_api.h"
#include "onnxruntime_cxx_api.h"

namespace KeyFrame {

ModelManager::ModelManager() {
    env_ = std::make_unique<Ort::Env>(ORT_LOGGING_LEVEL_WARNING, "KeyFrameModelManager");

    LOG_INFO("[ModelManager] Initialized");
}

void ModelManager::loadModel(const std::string& modelName, const std::string& modelPath,
                             FrameWorkType framework, const std::string& paramsPath) {
    std::lock_guard<std::mutex> lock(mtx_);

    // 检查是否已加载
    if (onnxSessions_.find(modelName) != onnxSessions_.end()) {
        LOG_INFO("[ModelManager] Model is already loaded: " + modelName);
        return;
    }

    ONNXSession::Config config;

    config.intraOpNumThreads = 4;

    config.optimizationLevel = GraphOptimizationLevel::ORT_ENABLE_ALL;

    auto session = std::make_unique<ONNXSession>(*env_, modelPath, config);

    onnxSessions_[modelName] = std::move(session);

    LOG_INFO("[ModelManager] Loaded model: " + modelName);
}

ONNXSession* ModelManager::getSession(const std::string& modelName) {
    std::lock_guard<std::mutex> lock(mtx_);

    auto it = onnxSessions_.find(modelName);
    if (it != onnxSessions_.end()) {
        return it->second.get();
    } else {
        LOG_ERROR("[ModelManager] Model not found: " + modelName);
        return nullptr;
    }
}

std::vector<std::vector<float>> ModelManager::runInference(
    const std::string& modelName, const std::vector<std::vector<float>>& inputs) {
    std::lock_guard<std::mutex> lock(mtx_);

    auto it = onnxSessions_.find(modelName);
    if (it == onnxSessions_.end()) {
        LOG_ERROR("[ModelManager] Cannot run inference, model not found: " + modelName);
        return {};
    }

    // 使用内存池优化：减少推理过程中的动态内存分配
    tensorBuffer_.reset();
    auto outputInfos = it->second->run(inputs, tensorBuffer_);

    // 将结果从内存池拷贝到返回的 vector
    std::vector<std::vector<float>> results;
    results.reserve(outputInfos.size());

    for (const auto& info : outputInfos) {
        results.emplace_back(info.data, info.data + info.elementCount);
    }

    return results;
}

std::vector<std::vector<float>> ModelManager::runInference(
    const std::string& modelName, const std::vector<std::vector<float>>& inputs,
    const std::vector<std::vector<int64_t>>& inputShapes) {
    std::lock_guard<std::mutex> lock(mtx_);

    auto it = onnxSessions_.find(modelName);
    if (it == onnxSessions_.end()) {
        LOG_ERROR("[ModelManager] Cannot run inference, model not found: " + modelName);
        return {};
    }

    // 使用内存池优化：减少推理过程中的动态内存分配
    tensorBuffer_.reset();
    auto outputInfos = it->second->run(inputs, inputShapes, tensorBuffer_);

    // 将结果从内存池拷贝到返回的 vector
    std::vector<std::vector<float>> results;
    results.reserve(outputInfos.size());

    for (const auto& info : outputInfos) {
        results.emplace_back(info.data, info.data + info.elementCount);
    }

    return results;
}

void ModelManager::warmUpModel() {
    std::lock_guard<std::mutex> lock(mtx_);

    for (auto& [name, session] : onnxSessions_) {
        session->warmUp();
        LOG_INFO("[ModelManager] Warmed up model: " + name);
    }
}

bool ModelManager::hasModel(const std::string& modelName) const {
    std::lock_guard<std::mutex> lock(mtx_);

    return onnxSessions_.find(modelName) != onnxSessions_.end();
}

std::vector<std::string> ModelManager::getLoadedModelNames() {
    std::lock_guard<std::mutex> lock(mtx_);

    std::vector<std::string> modelNames;
    for (const auto& [name, session] : onnxSessions_) {
        modelNames.push_back(name);
    }
    return modelNames;
}
};  // namespace KeyFrame