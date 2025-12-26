#include "ModelManager.h"

#include <memory>
#include <mutex>
#include <string>
#include <utility>
#include <vector>

#include "DataConverter.h"
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