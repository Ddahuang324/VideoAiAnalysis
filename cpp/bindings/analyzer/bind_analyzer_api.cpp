/**
 * @file bind_analyzer_api.cpp
 * @brief AnalyzerAPI 类的 Python 绑定
 */

#include <pybind11/attr.h>
#include <pybind11/cast.h>
#include <pybind11/functional.h>
#include <pybind11/gil.h>
#include <pybind11/pybind11.h>
#include <pybind11/pytypes.h>
#include <pybind11/stl.h>
#include <pyerrors.h>

#include <cstdint>
#include <string>

#include "Process/Analyzer/AnalyzerAPI.h"

namespace py = pybind11;

namespace Analyzer::Bindings {

// 创建 GIL 安全的 shared_ptr<py::function>，析构时自动获取 GIL
inline auto make_gil_safe_callback(py::function callback) {
    return std::shared_ptr<py::function>(
        new py::function(std::move(callback)),
        [](py::function* p) {
            if (p && Py_IsInitialized()) {
                py::gil_scoped_acquire gil;
                delete p;
            }
        });
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

        .def("analyze_video_file", &AnalyzerAPI::analyzeVideoFile, py::arg("file_path"),
             py::call_guard<py::gil_scoped_release>(),
             "启动离线视频文件分析\n\n"
             "读取指定视频文件并进行 AI 分析。此过程为异步执行。\n\n"
             "参数:\n"
             "    file_path (str): 视频文件的绝对路径\n\n"
             "返回:\n"
             "    bool: 成功启动返回 True，失败返回 False")

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
                auto safeCallback = make_gil_safe_callback(std::move(callback));
                api.setStatusCallback([safeCallback](AnalysisStatus status) {
                    if (Py_IsInitialized()) {
                        py::gil_scoped_acquire gil;
                        try { (*safeCallback)(status); } catch (const py::error_already_set&) { PyErr_Clear(); }
                    }
                });
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
                auto safeCallback = make_gil_safe_callback(std::move(callback));
                api.setKeyFrameCallback([safeCallback](int64_t frameIndex) {
                    if (Py_IsInitialized()) {
                        py::gil_scoped_acquire gil;
                        try { (*safeCallback)(frameIndex); } catch (const py::error_already_set&) { PyErr_Clear(); }
                    }
                });
            },
            py::arg("callback"),
            "设置关键帧检测回调\n\n"
            "回调签名: callback(frame_index: int) -> None\n\n"
            "示例:\n"
            "    def on_keyframe(frame_idx):\n"
            "        print(f'检测到关键帧: {frame_idx}')\n"
            "    api.set_keyframe_callback(on_keyframe)")

        .def(
            "set_keyframe_video_callback",
            [](AnalyzerAPI& api, py::function callback) {
                auto safeCallback = make_gil_safe_callback(std::move(callback));
                api.setKeyFrameVideoCallback([safeCallback](const std::string& videoPath) {
                    if (Py_IsInitialized()) {
                        py::gil_scoped_acquire gil;
                        try { (*safeCallback)(videoPath); } catch (const py::error_already_set&) { PyErr_Clear(); }
                    }
                });
            },
            py::arg("callback"),
            "设置关键帧视频生成完成回调\n\n"
            "当离线分析完成并成功生成关键帧视频后触发。\n\n"
            "回调签名: callback(video_path: str) -> None\n\n"
            "示例:\n"
            "    def on_keyframe_video(path):\n"
            "        print(f'关键帧视频生成: {path}')\n"
            "    api.set_keyframe_video_callback(on_keyframe_video)")

        // 实时分析控制
        .def("start_realtime_analysis", &AnalyzerAPI::startRealtimeAnalysis,
             py::call_guard<py::gil_scoped_release>(),
             "启动实时分析\\n\\n"
             "适合SNAPSHOT模式（1FPS），启动ZMQ帧接收和实时分析。\\n\\n"
             "返回:\\n"
             "    bool: 成功返回 True，失败返回 False")

        .def("stop_realtime_analysis", &AnalyzerAPI::stopRealtimeAnalysis,
             py::call_guard<py::gil_scoped_release>(),
             "停止实时分析\\n\\n"
             "停止ZMQ接收和分析线程。")

        .def("is_realtime_mode", &AnalyzerAPI::isRealtimeMode,
             "检查是否处于实时分析模式\\n\\n"
             "返回:\\n"
             "    bool: 实时模式返回 True，否则返回 False")

        // 分析模式管理
        .def("set_analysis_mode", &AnalyzerAPI::setAnalysisMode, py::arg("mode"),
             "设置分析模式\\n\\n"
             "参数:\\n"
             "    mode (AnalysisMode): REALTIME 或 OFFLINE")

        .def("get_analysis_mode", &AnalyzerAPI::getAnalysisMode,
             "获取当前分析模式\\n\\n"
             "返回:\\n"
             "    AnalysisMode: 当前模式")

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
