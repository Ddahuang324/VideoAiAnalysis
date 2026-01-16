#include "ModelManager.h"

#include <mutex>
#include <string>
#include <utility>
#include <vector>

#include "Log.h"
#include "ONNXSession.h"

namespace KeyFrame {

ModelManager::ModelManager() {
    env_ = std::make_unique<Ort::Env>(ORT_LOGGING_LEVEL_WARNING, "KeyFrameModelManager");
    LOG_INFO("[ModelManager] Initialized");
}

void ModelManager::loadModel(const std::string& modelName, const std::string& modelPath,
                             FrameWorkType framework, const std::string& paramsPath) {
    std::lock_guard<std::mutex> lock(mtx_);

    if (onnxSessions_.find(modelName) != onnxSessions_.end()) {
        LOG_INFO("[ModelManager] Model already loaded: " + modelName);
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
    }

    LOG_ERROR("[ModelManager] Model not found: " + modelName);
    return nullptr;
}

std::vector<std::vector<float>> ModelManager::runInference(
    const std::string& modelName, const std::vector<std::vector<float>>& inputs) {
    std::lock_guard<std::mutex> lock(mtx_);

    auto it = onnxSessions_.find(modelName);
    if (it == onnxSessions_.end()) {
        LOG_ERROR("[ModelManager] Cannot run inference, model not found: " + modelName);
        return {};
    }

    tensorBuffer_.reset();
    auto outputInfos = it->second->run(inputs, tensorBuffer_);

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

    tensorBuffer_.reset();
    auto outputInfos = it->second->run(inputs, inputShapes, tensorBuffer_);

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

}  // namespace KeyFrame
