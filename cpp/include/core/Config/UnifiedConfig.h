#pragma once

#include <filesystem>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

#include "ConfigBase.h"
#include "ConfigValidation.h"
#include "nlohmann/detail/macro_scope.hpp"
#include "nlohmann/json_fwd.hpp"

namespace Config {

// ==================== 基础配置组件 ====================

// ZMQ 通信配置 (可复用)
struct ZMQConfig {
    std::string endpoint = "tcp://localhost:5555";
    int timeoutMs = 100;
    int ioThreads = 1;

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(ZMQConfig, endpoint, timeoutMs, ioThreads)

    ValidationResult validate() const {
        ValidationResult result;
        VALIDATE_NOT_EMPTY(result, endpoint, "ZMQ endpoint");
        VALIDATE_POSITIVE(result, timeoutMs, "ZMQ timeout");
        VALIDATE_RANGE(result, ioThreads, 1, 16, "ZMQ IO threads");
        return result;
    }
};

// 模型路径配置 (可复用)
struct ModelPathsConfig {
    std::string basePath = "Models";
    std::string sceneModelPath;
    std::string motionModelPath;
    std::string textDetModelPath;
    std::string textRecModelPath;

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(ModelPathsConfig, basePath, sceneModelPath, motionModelPath,
                                   textDetModelPath, textRecModelPath)

    ValidationResult validate() const {
        ValidationResult result;
        VALIDATE_NOT_EMPTY(result, basePath, "Model base path");

        // 如果指定了模型路径,检查文件是否存在
        auto checkModelPath = [&result, this](const std::string& modelPath,
                                              const std::string& name) {
            if (!modelPath.empty()) {
                auto fullPath = std::filesystem::path(basePath) / modelPath;
                VALIDATE_FILE_EXISTS(result, fullPath.string(), name);
            }
        };

        checkModelPath(sceneModelPath, "Scene model");
        checkModelPath(motionModelPath, "Motion model");
        checkModelPath(textDetModelPath, "Text detection model");
        checkModelPath(textRecModelPath, "Text recognition model");

        return result;
    }
};

// ==================== Detector 配置 ====================

struct MotionDetectorConfig {
    float confidenceThreshold = 0.25f;
    float nmsThreshold = 0.45f;
    int inputWidth = 640;
    int maxTrackedObjects = 50;

    // ByteTrack 参数
    float trackHighThreshold = 0.6f;
    float trackLowThreshold = 0.1f;
    int trackBufferSize = 30;

    // 运动评分权重
    float pixelMotionWeight = 0.8f;
    float objectMotionWeight = 0.2f;

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(MotionDetectorConfig, confidenceThreshold, nmsThreshold,
                                   inputWidth, maxTrackedObjects, trackHighThreshold,
                                   trackLowThreshold, trackBufferSize, pixelMotionWeight,
                                   objectMotionWeight)

    ValidationResult validate() const {
        ValidationResult result;
        VALIDATE_RANGE(result, confidenceThreshold, 0.0f, 1.0f, "Confidence threshold");
        VALIDATE_RANGE(result, nmsThreshold, 0.0f, 1.0f, "NMS threshold");
        VALIDATE_POSITIVE(result, inputWidth, "Input width");
        VALIDATE_POSITIVE(result, maxTrackedObjects, "Max tracked objects");
        VALIDATE_RANGE(result, trackHighThreshold, 0.0f, 1.0f, "Track high threshold");
        VALIDATE_RANGE(result, trackLowThreshold, 0.0f, 1.0f, "Track low threshold");
        VALIDATE_POSITIVE(result, trackBufferSize, "Track buffer size");
        VALIDATE_RANGE(result, pixelMotionWeight, 0.0f, 1.0f, "Pixel motion weight");
        VALIDATE_RANGE(result, objectMotionWeight, 0.0f, 1.0f, "Object motion weight");

        // 验证权重和为 1.0
        float totalWeight = pixelMotionWeight + objectMotionWeight;
        ValidateWeightSum(result, totalWeight, "Motion weights");

        return result;
    }
};

struct SceneChangeDetectorConfig {
    float similarityThreshold = 0.8f;
    int featureDim = 1000;
    int inputSize = 224;
    bool enableCache = true;

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(SceneChangeDetectorConfig, similarityThreshold, featureDim,
                                   inputSize, enableCache)

    ValidationResult validate() const {
        ValidationResult result;
        VALIDATE_RANGE(result, similarityThreshold, 0.0f, 1.0f, "Similarity threshold");
        VALIDATE_POSITIVE(result, featureDim, "Feature dimension");
        VALIDATE_POSITIVE(result, inputSize, "Input size");
        return result;
    }
};

struct TextDetectorConfig {
    int detInputHeight = 960;
    int detInputWidth = 960;
    int recInputHeight = 48;
    int recInputWidth = 320;

    float detThreshold = 0.3f;
    float recThreshold = 0.5f;

    bool enableRecognition = false;

    float alpha = 0.6f;  // 文本区域覆盖率权重
    float beta = 0.4f;   // 文本变化率权重

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(TextDetectorConfig, detInputHeight, detInputWidth,
                                   recInputHeight, recInputWidth, detThreshold, recThreshold,
                                   enableRecognition, alpha, beta)

    ValidationResult validate() const {
        ValidationResult result;
        VALIDATE_POSITIVE(result, detInputHeight, "Detection input height");
        VALIDATE_POSITIVE(result, detInputWidth, "Detection input width");
        VALIDATE_POSITIVE(result, recInputHeight, "Recognition input height");
        VALIDATE_POSITIVE(result, recInputWidth, "Recognition input width");
        VALIDATE_RANGE(result, detThreshold, 0.0f, 1.0f, "Detection threshold");
        VALIDATE_RANGE(result, recThreshold, 0.0f, 1.0f, "Recognition threshold");
        VALIDATE_RANGE(result, alpha, 0.0f, 1.0f, "Alpha weight");
        VALIDATE_RANGE(result, beta, 0.0f, 1.0f, "Beta weight");

        float totalWeight = alpha + beta;
        ValidateWeightSum(result, totalWeight, "Text detector weights");

        return result;
    }
};

// ==================== Analyzer 配置 ====================

struct DynamicCalculatorConfig {
    std::vector<float> baseWeights = {0.45f, 0.2f, 0.35f};  // 场景、运动、文本
    float currentFrameWeight = 0.3f;
    float activationInfluence = 0.5f;
    int historyWindowSize = 30;
    float minWeight = 0.05f;
    float maxWeight = 0.7f;

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(DynamicCalculatorConfig, baseWeights, currentFrameWeight,
                                   activationInfluence, historyWindowSize, minWeight, maxWeight)

    ValidationResult validate() const {
        ValidationResult result;

        VALIDATE_VECTOR_SIZE(result, baseWeights, 3, "Base weights");

        if (baseWeights.size() == 3) {
            float sum = 0.0f;
            for (float w : baseWeights) {
                VALIDATE_RANGE(result, w, 0.0f, 1.0f, "Base weight");
                sum += w;
            }
            ValidateWeightSum(result, sum, "Base weights");
        }

        VALIDATE_RANGE(result, currentFrameWeight, 0.0f, 1.0f, "Current frame weight");
        VALIDATE_RANGE(result, activationInfluence, 0.0f, 1.0f, "Activation influence");
        VALIDATE_POSITIVE(result, historyWindowSize, "History window size");
        VALIDATE_RANGE(result, minWeight, 0.0f, 1.0f, "Min weight");
        VALIDATE_RANGE(result, maxWeight, 0.0f, 1.0f, "Max weight");

        VALIDATE_LESS_THAN(result, minWeight, maxWeight, "Min weight", "Max weight");

        return result;
    }
};

struct FrameScorerConfig {
    bool enableDynamicWeighting = true;
    bool enableSmoothing = true;
    int smoothingWindowSize = 3;
    float smoothingEMAAlpha = 0.6f;

    float sceneChangeBoost = 1.2f;
    float motionIncreaseBoost = 1.1f;
    float textIncreaseBoost = 1.1f;

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(FrameScorerConfig, enableDynamicWeighting, enableSmoothing,
                                   smoothingWindowSize, smoothingEMAAlpha, sceneChangeBoost,
                                   motionIncreaseBoost, textIncreaseBoost)

    ValidationResult validate() const {
        ValidationResult result;
        VALIDATE_POSITIVE(result, smoothingWindowSize, "Smoothing window size");
        VALIDATE_RANGE(result, smoothingEMAAlpha, 0.0f, 1.0f, "Smoothing EMA alpha");
        VALIDATE_RANGE(result, sceneChangeBoost, 1.0f, 2.0f, "Scene change boost");
        VALIDATE_RANGE(result, motionIncreaseBoost, 1.0f, 2.0f, "Motion increase boost");
        VALIDATE_RANGE(result, textIncreaseBoost, 1.0f, 2.0f, "Text increase boost");
        return result;
    }
};

struct KeyFrameDetectorConfig {
    int targetKeyFrameCount = 50;
    float targetCompressionRatio = 0.1f;

    int minKeyFrameCount = 5;
    int maxKeyFrameCount = 500;

    float minTemporalDistance = 1.0f;  // 秒

    bool useThresholdMode = true;
    float highQualityThreshold = 0.75f;
    float minScoreThreshold = 0.3f;
    bool alwaysIncludeSceneChanges = true;

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(KeyFrameDetectorConfig, targetKeyFrameCount,
                                   targetCompressionRatio, minKeyFrameCount, maxKeyFrameCount,
                                   minTemporalDistance, useThresholdMode, highQualityThreshold,
                                   minScoreThreshold, alwaysIncludeSceneChanges)

    ValidationResult validate() const {
        ValidationResult result;
        VALIDATE_POSITIVE(result, targetKeyFrameCount, "Target keyframe count");
        VALIDATE_RANGE(result, targetCompressionRatio, 0.0f, 1.0f, "Target compression ratio");
        VALIDATE_POSITIVE(result, minKeyFrameCount, "Min keyframe count");
        VALIDATE_POSITIVE(result, maxKeyFrameCount, "Max keyframe count");

        VALIDATE_LESS_THAN_OR_EQUAL(result, minKeyFrameCount, maxKeyFrameCount,
                                    "Min keyframe count", "Max keyframe count");

        WARN_IF(result,
                targetKeyFrameCount < minKeyFrameCount || targetKeyFrameCount > maxKeyFrameCount,
                "Target keyframe count is outside [min, max] range");

        VALIDATE_POSITIVE(result, minTemporalDistance, "Min temporal distance");
        VALIDATE_RANGE(result, highQualityThreshold, 0.0f, 1.0f, "High quality threshold");
        VALIDATE_RANGE(result, minScoreThreshold, 0.0f, 1.0f, "Min score threshold");

        return result;
    }
};

// ==================== Pipeline 配置 ====================

struct PipelineConfig {
    int analysisThreadCount = 1;
    int frameBufferSize = 100;
    int scoreBufferSize = 200;

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(PipelineConfig, analysisThreadCount, frameBufferSize,
                                   scoreBufferSize)

    ValidationResult validate() const {
        ValidationResult result;
        VALIDATE_POSITIVE(result, analysisThreadCount, "Analysis thread count");
        VALIDATE_POSITIVE(result, frameBufferSize, "Frame buffer size");
        VALIDATE_POSITIVE(result, scoreBufferSize, "Score buffer size");
        return result;
    }
};

// ==================== 顶层配置 ====================

// KeyFrameAnalyzerService 配置
struct KeyFrameAnalyzerConfig : public ConfigBase<KeyFrameAnalyzerConfig> {
    ZMQConfig zmqSubscriber;
    ZMQConfig zmqPublisher;

    ModelPathsConfig models;

    bool enableTextRecognition = false;

    // Detector 配置
    MotionDetectorConfig motionDetector;
    SceneChangeDetectorConfig sceneDetector;
    TextDetectorConfig textDetector;

    // Analyzer 配置
    DynamicCalculatorConfig dynamicCalculator;
    FrameScorerConfig frameScorer;
    KeyFrameDetectorConfig keyframeDetector;

    // Pipeline 配置
    PipelineConfig pipeline;

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(KeyFrameAnalyzerConfig, zmqSubscriber, zmqPublisher, models,
                                   enableTextRecognition, motionDetector, sceneDetector,
                                   textDetector, dynamicCalculator, frameScorer, keyframeDetector,
                                   pipeline)

    void fromJson(const nlohmann::json& j) override { *this = j.get<KeyFrameAnalyzerConfig>(); }

    nlohmann::json toJson() const override { return nlohmann::json(*this); }

    ValidationResult validate() const override {
        ValidationResult result;

        // 验证所有子配置
        auto validateSubConfig = [&result](const auto& config, const std::string& name) {
            auto subResult = config.validate();
            for (const auto& err : subResult.errors) {
                result.addError("[" + name + "] " + err);
            }
            for (const auto& warn : subResult.warnings) {
                result.addWarning("[" + name + "] " + warn);
            }
        };

        validateSubConfig(zmqSubscriber, "ZMQ Subscriber");
        validateSubConfig(zmqPublisher, "ZMQ Publisher");
        validateSubConfig(models, "Models");
        validateSubConfig(motionDetector, "Motion Detector");
        validateSubConfig(sceneDetector, "Scene Detector");
        validateSubConfig(textDetector, "Text Detector");
        validateSubConfig(dynamicCalculator, "Dynamic Calculator");
        validateSubConfig(frameScorer, "Frame Scorer");
        validateSubConfig(keyframeDetector, "Keyframe Detector");
        validateSubConfig(pipeline, "Pipeline");

        // 验证依赖关系
        if (enableTextRecognition && models.textRecModelPath.empty()) {
            result.addError("Text recognition is enabled but textRecModelPath is empty");
        }

        result.isValid = result.errors.empty();
        return result;
    }

    void merge(const IConfigBase& other) override {
        auto& otherConfig = dynamic_cast<const KeyFrameAnalyzerConfig&>(other);
        // 简单覆盖策略，可根据需求实现更复杂的合并逻辑
        if (!otherConfig.models.basePath.empty()) {
            models.basePath = otherConfig.models.basePath;
        }
        // 其他字段可根据需要添加合并逻辑
    }

    std::string getConfigName() const override { return "KeyFrameAnalyzerConfig"; }
};

// ==================== Foundation 配置 ====================

// ONNX 会话配置
struct ONNXSessionConfig {
    int intraOpNumThreads = 4;
    int interOpNumThreads = 2;
    bool enableCUDA = false;
    int cudaDeviceId = 0;
    int optimizationLevel = 3;  // GraphOptimizationLevel::ORT_ENABLE_ALL

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(ONNXSessionConfig, intraOpNumThreads, interOpNumThreads,
                                   enableCUDA, cudaDeviceId, optimizationLevel)

    ValidationResult validate() const {
        ValidationResult result;
        VALIDATE_POSITIVE(result, intraOpNumThreads, "Intra-op threads");
        VALIDATE_POSITIVE(result, interOpNumThreads, "Inter-op threads");
        VALIDATE_RANGE(result, cudaDeviceId, 0, 8, "CUDA device ID");
        return result;
    }
};

// ==================== Visualizer 配置 ====================

// 运动可视化配置
struct MotionVisualizerConfig {
    bool showBoundingBoxes = true;
    bool showTrackIds = true;
    bool showConfidence = false;
    bool showVelocityArrows = false;
    bool showTrackHistory = true;
    int historyLength = 10;

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(MotionVisualizerConfig, showBoundingBoxes, showTrackIds,
                                   showConfidence, showVelocityArrows, showTrackHistory,
                                   historyLength)

    ValidationResult validate() const {
        ValidationResult result;
        VALIDATE_POSITIVE(result, historyLength, "History length");
        return result;
    }
};

// ==================== Encoder 配置 ====================

// 视频编码器配置
struct VideoEncoderConfig {
    std::string outputFilePath = "output.mp4";
    int width = 1920;
    int height = 1080;
    int fps = 30;
    int bitrate = 4000000;  // 4 Mbps
    int crf = 23;
    std::string preset = "fast";
    std::string codec = "libx264";

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(VideoEncoderConfig, outputFilePath, width, height, fps, bitrate,
                                   crf, preset, codec)

    ValidationResult validate() const {
        ValidationResult result;
        VALIDATE_POSITIVE(result, width, "Video width");
        VALIDATE_POSITIVE(result, height, "Video height");
        VALIDATE_RANGE(result, fps, 1, 120, "Video FPS");
        VALIDATE_POSITIVE(result, bitrate, "Video bitrate");
        VALIDATE_RANGE(result, crf, 0, 51, "Video CRF");
        VALIDATE_NOT_EMPTY(result, preset, "Video preset");
        VALIDATE_NOT_EMPTY(result, codec, "Video codec");
        return result;
    }
};

// 音频编码器配置
struct AudioEncoderConfig {
    bool enabled = true;
    int sampleRate = 48000;
    int channels = 2;
    int bitrate = 128000;  // 128 kbps
    std::string codec = "aac";

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(AudioEncoderConfig, enabled, sampleRate, channels, bitrate,
                                   codec)

    ValidationResult validate() const {
        ValidationResult result;
        if (enabled) {
            VALIDATE_RANGE(result, sampleRate, 8000, 192000, "Audio sample rate");
            VALIDATE_RANGE(result, channels, 1, 8, "Audio channels");
            VALIDATE_POSITIVE(result, bitrate, "Audio bitrate");
            VALIDATE_NOT_EMPTY(result, codec, "Audio codec");
        }
        return result;
    }
};

// 完整编码器配置 (视频 + 音频)
struct EncoderConfig {
    VideoEncoderConfig video;
    AudioEncoderConfig audio;

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(EncoderConfig, video, audio)

    ValidationResult validate() const {
        ValidationResult result;
        auto videoResult = video.validate();
        auto audioResult = audio.validate();

        result.errors.insert(result.errors.end(), videoResult.errors.begin(),
                             videoResult.errors.end());
        result.warnings.insert(result.warnings.end(), videoResult.warnings.begin(),
                               videoResult.warnings.end());
        result.errors.insert(result.errors.end(), audioResult.errors.begin(),
                             audioResult.errors.end());
        result.warnings.insert(result.warnings.end(), audioResult.warnings.begin(),
                               audioResult.warnings.end());

        result.isValid = result.errors.empty();
        return result;
    }
};

// Recorder 配置
struct RecorderConfig : public ConfigBase<RecorderConfig> {
    ZMQConfig zmqPublisher;
    VideoEncoderConfig video;
    AudioEncoderConfig audio;

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(RecorderConfig, zmqPublisher, video, audio)

    void fromJson(const nlohmann::json& j) override { *this = j.get<RecorderConfig>(); }

    nlohmann::json toJson() const override { return nlohmann::json(*this); }

    ValidationResult validate() const override {
        ValidationResult result;

        auto validateSubConfig = [&result](const auto& config, const std::string& name) {
            auto subResult = config.validate();
            for (const auto& err : subResult.errors) {
                result.addError("[" + name + "] " + err);
            }
            for (const auto& warn : subResult.warnings) {
                result.addWarning("[" + name + "] " + warn);
            }
        };

        validateSubConfig(zmqPublisher, "ZMQ Publisher");
        validateSubConfig(video, "Video Encoder");
        validateSubConfig(audio, "Audio Encoder");

        result.isValid = result.errors.empty();
        return result;
    }

    void merge(const IConfigBase& other) override {
        // 简单覆盖策略
        *this = dynamic_cast<const RecorderConfig&>(other);
    }

    std::string getConfigName() const override { return "RecorderConfig"; }
};

}  // namespace Config
