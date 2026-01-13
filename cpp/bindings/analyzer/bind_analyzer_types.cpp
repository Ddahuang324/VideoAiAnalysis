/**
 * @file bind_analyzer_types.cpp
 * @brief AnalysisStatus 枚举和 AnalysisStats 结构体的 Python 绑定
 */

#include <pybind11/attr.h>
#include <pybind11/pybind11.h>
#include <pybind11/pytypes.h>
#include <pybind11/stl.h>

#include <string>

#include "Process/Analyzer/AnalyzerAPI.h"

namespace py = pybind11;

namespace Analyzer::Bindings {

void bind_analyzer_types(py::module& m) {
    using namespace Analyzer;

    // 绑定 AnalysisStatus 枚举
    py::enum_<AnalysisStatus>(m, "AnalysisStatus", py::arithmetic(), "分析状态枚举")
        .value("IDLE", AnalysisStatus::IDLE, "空闲状态")
        .value("INITIALIZING", AnalysisStatus::INITIALIZING, "初始化中")
        .value("RUNNING", AnalysisStatus::RUNNING, "运行中")
        .value("STOPPING", AnalysisStatus::STOPPING, "停止中")
        .value("ERROR", AnalysisStatus::ERROR, "错误状态")
        .export_values()
        .def("__str__",
             [](AnalysisStatus status) {
                 switch (status) {
                     case AnalysisStatus::IDLE:
                         return "IDLE";
                     case AnalysisStatus::INITIALIZING:
                         return "INITIALIZING";
                     case AnalysisStatus::RUNNING:
                         return "RUNNING";
                     case AnalysisStatus::STOPPING:
                         return "STOPPING";
                     case AnalysisStatus::ERROR:
                         return "ERROR";
                     default:
                         return "UNKNOWN";
                 }
             })
        .def("__repr__", [](AnalysisStatus status) {
            return "<AnalysisStatus." + std::string([](AnalysisStatus s) {
                       switch (s) {
                           case AnalysisStatus::IDLE:
                               return "IDLE";
                           case AnalysisStatus::INITIALIZING:
                               return "INITIALIZING";
                           case AnalysisStatus::RUNNING:
                               return "RUNNING";
                           case AnalysisStatus::STOPPING:
                               return "STOPPING";
                           case AnalysisStatus::ERROR:
                               return "ERROR";
                           default:
                               return "UNKNOWN";
                       }
                   }(status)) +
                   ">";
        });

    // 绑定 KeyFrameRecord
    py::class_<KeyFrameRecord>(m, "KeyFrameRecord", "关键帧记录")
        .def(py::init<>(), "默认构造函数")
        .def_readwrite("frame_index", &KeyFrameRecord::frameIndex, "帧索引")
        .def_readwrite("score", &KeyFrameRecord::score, "关键帧评分")
        .def_readwrite("timestamp", &KeyFrameRecord::timestamp, "时间戳")
        .def("__repr__",
             [](const KeyFrameRecord& r) {
                 return "<KeyFrameRecord frame=" + std::to_string(r.frameIndex) +
                        " score=" + std::to_string(r.score) +
                        " time=" + std::to_string(r.timestamp) + ">";
             })
        // 便捷方法
        .def(
            "to_dict",
            [](const KeyFrameRecord& r) {
                py::dict dict;
                dict["frame_index"] = r.frameIndex;
                dict["score"] = r.score;
                dict["timestamp"] = r.timestamp;
                return dict;
            },
            "将记录转换为 Python 字典");

    // 绑定 AnalysisStats::ConfigSummary
    py::class_<AnalysisStats::ConfigSummary>(m, "ConfigSummary", "配置摘要")
        .def(py::init<>())
        .def_readwrite("text_recognition_enabled",
                       &AnalysisStats::ConfigSummary::textRecognitionEnabled, "是否启用文本识别")
        .def_readwrite("thread_count", &AnalysisStats::ConfigSummary::threadCount, "线程数")
        .def_readwrite("active_model_info", &AnalysisStats::ConfigSummary::activeModelInfo,
                       "活动模型信息")
        .def("__repr__", [](const AnalysisStats::ConfigSummary& s) {
            return "<ConfigSummary text_rec=" +
                   std::string(s.textRecognitionEnabled ? "enabled" : "disabled") +
                   " threads=" + std::to_string(s.threadCount) + ">";
        });

    // 绑定 AnalysisStats
    py::class_<AnalysisStats>(m, "AnalysisStats", "分析统计信息")
        .def(py::init<>())
        .def_readwrite("received_frame_count", &AnalysisStats::receivedFrameCount, "接收的总帧数")
        .def_readwrite("analyzed_frame_count", &AnalysisStats::analyzedFrameCount, "已分析的帧数")
        .def_readwrite("keyframe_count", &AnalysisStats::keyframeCount, "检测到的关键帧数")
        .def_readwrite("latest_keyframes", &AnalysisStats::latestKeyFrames, "最近的关键帧列表")
        .def_readwrite("active_config", &AnalysisStats::activeConfig, "当前运行配置")
        .def_readwrite("avg_processing_time", &AnalysisStats::avgProcessingTime,
                       "平均处理时间 (毫秒)")
        .def("__repr__",
             [](const AnalysisStats& stats) {
                 return "<AnalysisStats received=" + std::to_string(stats.receivedFrameCount) +
                        " analyzed=" + std::to_string(stats.analyzedFrameCount) +
                        " keyframes=" + std::to_string(stats.keyframeCount) +
                        " avg_time=" + std::to_string(stats.avgProcessingTime) + "ms>";
             })
        // 便捷方法
        .def(
            "to_dict",
            [](const AnalysisStats& stats) {
                py::dict dict;
                dict["received_frame_count"] = stats.receivedFrameCount;
                dict["analyzed_frame_count"] = stats.analyzedFrameCount;
                dict["keyframe_count"] = stats.keyframeCount;
                dict["avg_processing_time"] = stats.avgProcessingTime;

                // 转换 latest_keyframes 列表
                py::list keyframes;
                for (const auto& kf : stats.latestKeyFrames) {
                    py::dict kf_dict;
                    kf_dict["frame_index"] = kf.frameIndex;
                    kf_dict["score"] = kf.score;
                    kf_dict["timestamp"] = kf.timestamp;
                    keyframes.append(kf_dict);
                }
                dict["latest_keyframes"] = keyframes;

                // 转换 config
                py::dict config_dict;
                config_dict["text_recognition_enabled"] = stats.activeConfig.textRecognitionEnabled;
                config_dict["thread_count"] = stats.activeConfig.threadCount;
                config_dict["active_model_info"] = stats.activeConfig.activeModelInfo;
                dict["active_config"] = config_dict;

                return dict;
            },
            "将统计信息转换为 Python 字典");
}

}  // namespace Analyzer::Bindings
