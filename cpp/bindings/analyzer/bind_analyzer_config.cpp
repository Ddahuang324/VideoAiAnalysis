/**
 * @file bind_analyzer_config.cpp
 * @brief AnalyzerConfig 及相关配置类的 Python 绑定
 */

#include <pybind11/attr.h>
#include <pybind11/cast.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include <string>

#include "core/Config/UnifiedConfig.h"
#include "nlohmann/json_fwd.hpp"

namespace py = pybind11;

namespace Analyzer::Bindings {

void bind_analyzer_config(py::module& m) {
    using namespace Config;

    // 绑定 ZMQConfig (使用 module_local 避免与 recorder_module 冲突)
    py::class_<ZMQConfig>(m, "ZMQConfig", py::module_local(), "ZMQ 通信配置")
        .def(py::init<>())
        .def_readwrite("endpoint", &ZMQConfig::endpoint, "ZMQ 端点地址")
        .def_readwrite("timeout_ms", &ZMQConfig::timeoutMs, "超时时间 (毫秒)")
        .def_readwrite("io_threads", &ZMQConfig::ioThreads, "IO 线程数")
        .def("validate", &ZMQConfig::validate, "验证配置有效性");

    // 绑定 ModelPathsConfig
    py::class_<ModelPathsConfig>(m, "ModelPathsConfig", "模型路径配置")
        .def(py::init<>())
        .def_readwrite("base_path", &ModelPathsConfig::basePath, "模型基础路径")
        .def_readwrite("scene_model_path", &ModelPathsConfig::sceneModelPath, "场景检测模型路径")
        .def_readwrite("motion_model_path", &ModelPathsConfig::motionModelPath, "运动检测模型路径")
        .def_readwrite("text_det_model_path", &ModelPathsConfig::textDetModelPath,
                       "文本检测模型路径")
        .def_readwrite("text_rec_model_path", &ModelPathsConfig::textRecModelPath,
                       "文本识别模型路径")
        .def("validate", &ModelPathsConfig::validate, "验证配置有效性")
        .def("__repr__", [](const ModelPathsConfig& c) {
            return "<ModelPathsConfig base=" + c.basePath + ">";
        });

    // 绑定 MotionDetectorConfig
    py::class_<MotionDetectorConfig>(m, "MotionDetectorConfig", "运动检测器配置")
        .def(py::init<>())
        .def_readwrite("confidence_threshold", &MotionDetectorConfig::confidenceThreshold,
                       "置信度阈值")
        .def_readwrite("nms_threshold", &MotionDetectorConfig::nmsThreshold, "NMS 阈值")
        .def_readwrite("input_width", &MotionDetectorConfig::inputWidth, "输入宽度")
        .def_readwrite("max_tracked_objects", &MotionDetectorConfig::maxTrackedObjects,
                       "最大跟踪对象数")
        .def_readwrite("track_high_threshold", &MotionDetectorConfig::trackHighThreshold,
                       "跟踪高阈值")
        .def_readwrite("track_low_threshold", &MotionDetectorConfig::trackLowThreshold,
                       "跟踪低阈值")
        .def_readwrite("track_buffer_size", &MotionDetectorConfig::trackBufferSize,
                       "跟踪缓冲区大小")
        .def_readwrite("pixel_motion_weight", &MotionDetectorConfig::pixelMotionWeight,
                       "像素运动权重")
        .def_readwrite("object_motion_weight", &MotionDetectorConfig::objectMotionWeight,
                       "对象运动权重")
        .def("validate", &MotionDetectorConfig::validate);

    // 绑定 SceneChangeDetectorConfig
    py::class_<SceneChangeDetectorConfig>(m, "SceneChangeDetectorConfig", "场景变化检测器配置")
        .def(py::init<>())
        .def_readwrite("similarity_threshold", &SceneChangeDetectorConfig::similarityThreshold,
                       "相似度阈值")
        .def_readwrite("feature_dim", &SceneChangeDetectorConfig::featureDim, "特征维度")
        .def_readwrite("input_size", &SceneChangeDetectorConfig::inputSize, "输入尺寸")
        .def_readwrite("enable_cache", &SceneChangeDetectorConfig::enableCache, "启用缓存")
        .def("validate", &SceneChangeDetectorConfig::validate);

    // 绑定 TextDetectorConfig
    py::class_<TextDetectorConfig>(m, "TextDetectorConfig", "文本检测器配置")
        .def(py::init<>())
        .def_readwrite("det_input_height", &TextDetectorConfig::detInputHeight, "检测输入高度")
        .def_readwrite("det_input_width", &TextDetectorConfig::detInputWidth, "检测输入宽度")
        .def_readwrite("rec_input_height", &TextDetectorConfig::recInputHeight, "识别输入高度")
        .def_readwrite("rec_input_width", &TextDetectorConfig::recInputWidth, "识别输入宽度")
        .def_readwrite("det_threshold", &TextDetectorConfig::detThreshold, "检测阈值")
        .def_readwrite("rec_threshold", &TextDetectorConfig::recThreshold, "识别阈值")
        .def_readwrite("enable_recognition", &TextDetectorConfig::enableRecognition, "启用识别")
        .def_readwrite("alpha", &TextDetectorConfig::alpha, "文本区域覆盖率权重")
        .def_readwrite("beta", &TextDetectorConfig::beta, "文本变化率权重")
        .def("validate", &TextDetectorConfig::validate);

    // 绑定 DynamicCalculatorConfig
    py::class_<DynamicCalculatorConfig>(m, "DynamicCalculatorConfig", "动态权重计算配置")
        .def(py::init<>())
        .def_readwrite("base_weights", &DynamicCalculatorConfig::baseWeights,
                       "基础权重向量 [场景, 运动, 文本]")
        .def_readwrite("current_frame_weight", &DynamicCalculatorConfig::currentFrameWeight,
                       "当前帧权重")
        .def_readwrite("activation_influence", &DynamicCalculatorConfig::activationInfluence,
                       "激活影响系数")
        .def_readwrite("history_window_size", &DynamicCalculatorConfig::historyWindowSize,
                       "历史窗口大小")
        .def_readwrite("min_weight", &DynamicCalculatorConfig::minWeight, "最小权重限制")
        .def_readwrite("max_weight", &DynamicCalculatorConfig::maxWeight, "最大权重限制")
        .def("validate", &DynamicCalculatorConfig::validate);

    // 绑定 FrameScorerConfig
    py::class_<FrameScorerConfig>(m, "FrameScorerConfig", "帧评分配置")
        .def(py::init<>())
        .def_readwrite("enable_dynamic_weighting", &FrameScorerConfig::enableDynamicWeighting,
                       "是否启用动态权重")
        .def_readwrite("enable_smoothing", &FrameScorerConfig::enableSmoothing, "是否启用平滑处理")
        .def_readwrite("smoothing_window_size", &FrameScorerConfig::smoothingWindowSize,
                       "平滑窗口大小")
        .def_readwrite("smoothing_ema_alpha", &FrameScorerConfig::smoothingEMAAlpha, "EMA 平滑系数")
        .def_readwrite("scene_change_boost", &FrameScorerConfig::sceneChangeBoost,
                       "场景切换得分增益")
        .def_readwrite("motion_increase_boost", &FrameScorerConfig::motionIncreaseBoost,
                       "运动增加得分增益")
        .def_readwrite("text_increase_boost", &FrameScorerConfig::textIncreaseBoost,
                       "文本增加得分增益")
        .def("validate", &FrameScorerConfig::validate);

    // 绑定 KeyFrameDetectorConfig
    py::class_<KeyFrameDetectorConfig>(m, "KeyFrameDetectorConfig", "关键帧提取配置")
        .def(py::init<>())
        .def_readwrite("target_keyframe_count", &KeyFrameDetectorConfig::targetKeyFrameCount,
                       "目标关键帧数")
        .def_readwrite("target_compression_ratio", &KeyFrameDetectorConfig::targetCompressionRatio,
                       "目标压缩率")
        .def_readwrite("min_keyframe_count", &KeyFrameDetectorConfig::minKeyFrameCount,
                       "最小关键帧数")
        .def_readwrite("max_keyframe_count", &KeyFrameDetectorConfig::maxKeyFrameCount,
                       "最大关键帧数")
        .def_readwrite("min_temporal_distance", &KeyFrameDetectorConfig::minTemporalDistance,
                       "最小时间间隔(秒)")
        .def_readwrite("use_threshold_mode", &KeyFrameDetectorConfig::useThresholdMode,
                       "是否使用阈值模式")
        .def_readwrite("high_quality_threshold", &KeyFrameDetectorConfig::highQualityThreshold,
                       "高质量阈值")
        .def_readwrite("min_score_threshold", &KeyFrameDetectorConfig::minScoreThreshold,
                       "最低分数阈值")
        .def_readwrite("always_include_scene_changes",
                       &KeyFrameDetectorConfig::alwaysIncludeSceneChanges, "始终包含场景切换")
        .def("validate", &KeyFrameDetectorConfig::validate);

    // 绑定 PipelineConfig
    py::class_<PipelineConfig>(m, "PipelineConfig", "分析流水线配置")
        .def(py::init<>())
        .def_readwrite("analysis_thread_count", &PipelineConfig::analysisThreadCount, "分析线程数")
        .def_readwrite("frame_buffer_size", &PipelineConfig::frameBufferSize, "帧缓冲区大小")
        .def_readwrite("score_buffer_size", &PipelineConfig::scoreBufferSize, "评分缓冲区大小")
        .def("validate", &PipelineConfig::validate);

    // 绑定 KeyFrameAnalyzerConfig (统一配置)
    py::class_<KeyFrameAnalyzerConfig>(m, "AnalyzerConfig", "分析器统一配置")
        .def(py::init<>())
        .def_readwrite("zmq_subscriber", &KeyFrameAnalyzerConfig::zmqSubscriber, "ZMQ 订阅器配置")
        .def_readwrite("zmq_publisher", &KeyFrameAnalyzerConfig::zmqPublisher, "ZMQ 发布器配置")
        .def_readwrite("models", &KeyFrameAnalyzerConfig::models, "模型路径配置")
        .def_readwrite("enable_text_recognition", &KeyFrameAnalyzerConfig::enableTextRecognition,
                       "是否启用文本识别")
        .def_readwrite("motion_detector", &KeyFrameAnalyzerConfig::motionDetector, "运动检测器配置")
        .def_readwrite("scene_detector", &KeyFrameAnalyzerConfig::sceneDetector, "场景检测器配置")
        .def_readwrite("text_detector", &KeyFrameAnalyzerConfig::textDetector, "文本检测器配置")
        .def_readwrite("dynamic_calculator", &KeyFrameAnalyzerConfig::dynamicCalculator,
                       "动态计算器配置")
        .def_readwrite("frame_scorer", &KeyFrameAnalyzerConfig::frameScorer, "帧评分器配置")
        .def_readwrite("keyframe_detector", &KeyFrameAnalyzerConfig::keyframeDetector,
                       "关键帧检测器配置")
        .def_readwrite("pipeline", &KeyFrameAnalyzerConfig::pipeline, "流水线配置")
        .def("validate", &KeyFrameAnalyzerConfig::validate, "验证配置有效性")
        .def("load_from_file", &KeyFrameAnalyzerConfig::loadFromFile, py::arg("filepath"),
             "从 JSON 文件加载配置")
        .def("save_to_file", &KeyFrameAnalyzerConfig::saveToFile, py::arg("filepath"),
             "保存配置到 JSON 文件")
        .def(
            "to_json", [](const KeyFrameAnalyzerConfig& c) { return c.toJson().dump(); },
            "将配置转换为 JSON 字符串")
        .def(
            "from_json",
            [](KeyFrameAnalyzerConfig& c, const std::string& jsonStr) {
                c.fromJson(nlohmann::json::parse(jsonStr));
            },
            py::arg("json_str"), "从 JSON 字符串加载配置")
        .def("__repr__", [](const KeyFrameAnalyzerConfig& c) {
            return "<AnalyzerConfig text_rec=" +
                   std::string(c.enableTextRecognition ? "enabled" : "disabled") +
                   " threads=" + std::to_string(c.pipeline.analysisThreadCount) + ">";
        });

    // 工厂函数: 创建默认分析器配置
    m.def(
        "default_analyzer_config",
        []() -> KeyFrameAnalyzerConfig {
            KeyFrameAnalyzerConfig config;

            // ZMQ 配置
            config.zmqSubscriber.endpoint = "tcp://localhost:5555";
            config.zmqSubscriber.timeoutMs = 100;
            config.zmqSubscriber.ioThreads = 1;

            config.zmqPublisher.endpoint = "tcp://*:5556";
            config.zmqPublisher.timeoutMs = 100;
            config.zmqPublisher.ioThreads = 1;

            // 模型路径
            config.models.basePath = "Models";
            config.models.sceneModelPath = "MobileNet-v3-Small.onnx";
            config.models.motionModelPath = "yolov8n.onnx";

            // 文本识别
            config.enableTextRecognition = false;

            // 流水线
            config.pipeline.analysisThreadCount = 4;
            config.pipeline.frameBufferSize = 100;
            config.pipeline.scoreBufferSize = 200;

            return config;
        },
        "创建默认分析器配置\n\n"
        "返回:\n"
        "    AnalyzerConfig: 带有默认值的配置对象");
}

}  // namespace Analyzer::Bindings
