#pragma once

#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

#include "ONNXSession.h"
#include "TensorBuffer.h"
#include "onnxruntime_cxx_api.h"

namespace KeyFrame {

class ModelManager {
public:
    enum class FrameWorkType {
        ONNXRuntime,
        // Future frameworks can be added here
    };

    static ModelManager& GetInstance() {
        static ModelManager instance;
        return instance;
    }

    void loadModel(const std::string& modelName, const std::string& modelPath,
                   FrameWorkType framework, const std::string& prtamspath = "");

    ONNXSession* getSession(const std::string& modelName);

    /**
     * @brief 统一推理接口
     * @param modelName 模型名称
     * @param inputs 输入数据 (Batch, Data)
     * @return 推理结果 (Batch, Data)
     */
    std::vector<std::vector<float>> runInference(const std::string& modelName,
                                                 const std::vector<std::vector<float>>& inputs);

    std::vector<std::vector<float>> runInference(
        const std::string& modelName, const std::vector<std::vector<float>>& inputs,
        const std::vector<std::vector<int64_t>>& inputShapes);

    void warmUpModel();

    bool hasModel(const std::string& modelName) const;

    std::vector<std::string> getLoadedModelNames();

    ModelManager(const ModelManager&) = delete;
    ModelManager& operator=(const ModelManager&) = delete;

private:
    ModelManager();
    ~ModelManager() = default;

    std::unique_ptr<Ort::Env> env_;
    mutable std::mutex mtx_;
    std::unordered_map<std::string, std::unique_ptr<ONNXSession>> onnxSessions_;
    TensorBuffer tensorBuffer_;  // 共享内存池，减少推理时的动态分配
};

};  // namespace KeyFrame