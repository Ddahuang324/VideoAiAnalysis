/**
 * @file bind_analyzer_api.cpp
 * @brief AnalyzerAPI 类的 Python 绑定
 */

#include <pybind11/functional.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "Process/Analyzer/AnalyzerAPI.h"


namespace py = pybind11;

namespace Analyzer::Bindings {

// GIL 管理辅助函数
template <typename Func>
Func wrap_callback_with_gil(Func callback) {
    if (!callback) {
        return nullptr;
    }

    return [callback](auto&&... args) {
        py::gil_scoped_acquire gil;
        try {
            callback(std::forward<decltype(args)>(args)...);
        } catch (const py::error_already_set& e) {
            PyErr_Clear();
            // TODO: 添加日志记录
        }
    };
}

void bind_analyzer_api(py::module& m) {
    using namespace Analyzer;

    py::class_<AnalyzerAPI>(m, "AnalyzerAPI",
                            "分析进程 API\n\n"
                            "提供 AI 视频分析的完整功能，包括帧接收、关键帧检测、结果发布等。")

        .def(py::init<>(), "默认构造函数")

        // 生命周期管理
        .def("initialize", &AnalyzerAPI::initialize, py::arg("config"),
             py::call_guard<py::gil_scoped_release>(),
             "初始化分析器\n\n"
             "参数:\n"
             "    config (AnalyzerConfig): 分析器配置对象\n\n"
             "返回:\n"
             "    bool: 成功返回 True，失败返回 False")

        .def("start", &AnalyzerAPI::start, py::call_guard<py::gil_scoped_release>(),
             "启动分析\n\n"
             "开始订阅帧数据并进行 AI 分析。\n\n"
             "返回:\n"
             "    bool: 成功返回 True，失败返回 False")

        .def("stop", &AnalyzerAPI::stop, py::call_guard<py::gil_scoped_release>(),
             "停止分析\n\n"
             "停止接收帧并刷新所有待处理数据。\n\n"
             "返回:\n"
             "    bool: 成功返回 True，失败返回 False")

        .def("shutdown", &AnalyzerAPI::shutdown, py::call_guard<py::gil_scoped_release>(),
             "关闭分析器\n\n"
             "释放所有资源，包括 ONNX 模型。")

        // 状态查询
        .def("get_status", &AnalyzerAPI::getStatus,
             "获取当前分析状态\n\n"
             "返回:\n"
             "    AnalysisStatus: 当前状态枚举值")

        .def("get_stats", &AnalyzerAPI::getStats,
             "获取分析统计信息\n\n"
             "返回:\n"
             "    AnalysisStats: 包含接收帧数、关键帧数等统计")

        .def("get_last_error", &AnalyzerAPI::getLastError,
             "获取最后一次错误信息\n\n"
             "返回:\n"
             "    str: 错误描述字符串")

        // 回调设置
        .def(
            "set_status_callback",
            [](AnalyzerAPI& api, py::function callback) {
                using CallbackType = AnalyzerAPI::StatusCallback;
                auto wrapped = wrap_callback_with_gil<CallbackType>(
                    [callback](AnalysisStatus status) { callback(status); });
                api.setStatusCallback(wrapped);
            },
            py::arg("callback"),
            "设置状态变更回调\n\n"
            "回调签名: callback(status: AnalysisStatus) -> None\n\n"
            "示例:\n"
            "    def on_status_change(status):\n"
            "        print(f'状态变更: {status}')\n"
            "    api.set_status_callback(on_status_change)")

        .def(
            "set_keyframe_callback",
            [](AnalyzerAPI& api, py::function callback) {
                using CallbackType = AnalyzerAPI::KeyFrameCallback;
                auto wrapped = wrap_callback_with_gil<CallbackType>(
                    [callback](int64_t frameIndex) { callback(frameIndex); });
                api.setKeyFrameCallback(wrapped);
            },
            py::arg("callback"),
            "设置关键帧检测回调\n\n"
            "回调签名: callback(frame_index: int) -> None\n\n"
            "示例:\n"
            "    def on_keyframe(frame_idx):\n"
            "        print(f'检测到关键帧: {frame_idx}')\n"
            "    api.set_keyframe_callback(on_keyframe)")

        // Python 属性
        .def_property_readonly("status", &AnalyzerAPI::getStatus, "当前分析状态 (只读属性)")
        .def_property_readonly("stats", &AnalyzerAPI::getStats, "分析统计信息 (只读属性)")
        .def_property_readonly("last_error", &AnalyzerAPI::getLastError, "最后错误信息 (只读属性)")

        // 便捷属性
        .def_property_readonly(
            "received_frame_count",
            [](const AnalyzerAPI& api) { return api.getStats().receivedFrameCount; },
            "接收的帧数 (只读属性)")
        .def_property_readonly(
            "analyzed_frame_count",
            [](const AnalyzerAPI& api) { return api.getStats().analyzedFrameCount; },
            "已分析的帧数 (只读属性)")
        .def_property_readonly(
            "keyframe_count", [](const AnalyzerAPI& api) { return api.getStats().keyframeCount; },
            "关键帧数 (只读属性)")
        .def_property_readonly(
            "is_running",
            [](const AnalyzerAPI& api) { return api.getStatus() == AnalysisStatus::RUNNING; },
            "是否正在运行 (只读属性)")

        // 上下文管理器
        .def("__enter__", [](AnalyzerAPI& api) -> AnalyzerAPI& { return api; })
        .def("__exit__",
             [](AnalyzerAPI& api, py::object, py::object, py::object) {
                 if (api.getStatus() == AnalysisStatus::RUNNING) {
                     py::gil_scoped_release release;
                     api.stop();
                 }
                 api.shutdown();
                 return false;
             })

        .def("__repr__",
             [](const AnalyzerAPI& api) {
                 auto stats = api.getStats();
                 return "<AnalyzerAPI status=" + std::to_string(static_cast<int>(api.getStatus())) +
                        " keyframes=" + std::to_string(stats.keyframeCount) + ">";
             })

        .def("__str__", [](const AnalyzerAPI& api) {
            auto stats = api.getStats();
            return "AnalyzerAPI(status=" + std::string([](AnalysisStatus s) {
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
                   }(api.getStatus())) +
                   ", keyframes=" + std::to_string(stats.keyframeCount) +
                   ", analyzed=" + std::to_string(stats.analyzedFrameCount) + ")";
        });
}

}  // namespace Analyzer::Bindings
