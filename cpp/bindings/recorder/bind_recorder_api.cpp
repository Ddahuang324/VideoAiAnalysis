/**
 * @file bind_recorder_api.cpp
 * @brief RecorderAPI 类的 Python 绑定
 */

#include <pybind11/attr.h>
#include <pybind11/cast.h>
#include <pybind11/functional.h>
#include <pybind11/gil.h>
#include <pybind11/pybind11.h>
#include <pybind11/pytypes.h>
#include <pybind11/stl.h>
#include <pyerrors.h>

#include <string>

#include "Process/Recorder/RecorderAPI.h"

namespace py = pybind11;

namespace Recorder::Bindings {

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

void bind_recorder_api(py::module& m) {
    using namespace Recorder;

    // 绑定 RecorderMode 枚举
    py::enum_<RecorderMode>(m, "RecorderMode", "录制模式")
        .value("VIDEO", RecorderMode::VIDEO, "视频模式（连续录制）")
        .value("SNAPSHOT", RecorderMode::SNAPSHOT, "快照模式（单帧录制）")
        .export_values();

    py::class_<RecorderAPI>(
        m, "RecorderAPI",
        "录制进程 API\n\n"
        "提供视频录制的完整生命周期管理，包括初始化、启动、暂停、恢复、停止等功能。\n"
        "支持状态查询和回调机制。")

        // 构造与析构
        .def(py::init<>(), "默认构造函数")

        // 生命周期管理
        .def("initialize", &RecorderAPI::initialize, py::arg("config"),
             py::call_guard<py::gil_scoped_release>(),
             "初始化录制器\n\n"
             "参数:\n"
             "    config (RecorderConfig): 录制配置对象\n\n"
             "返回:\n"
             "    bool: 成功返回 True，失败返回 False")

        .def("start", &RecorderAPI::start, py::call_guard<py::gil_scoped_release>(),
             "启动录制\n\n"
             "必须在 initialize() 成功后调用。\n\n"
             "返回:\n"
             "    bool: 成功返回 True，失败返回 False")

        .def("pause", &RecorderAPI::pause, py::call_guard<py::gil_scoped_release>(),
             "暂停录制\n\n"
             "仅在 RECORDING 状态下有效。\n\n"
             "返回:\n"
             "    bool: 成功返回 True，失败返回 False")

        .def("resume", &RecorderAPI::resume, py::call_guard<py::gil_scoped_release>(),
             "恢复录制\n\n"
             "仅在 PAUSED 状态下有效。\n\n"
             "返回:\n"
             "    bool: 成功返回 True，失败返回 False")

        .def("stop", &RecorderAPI::stop, py::call_guard<py::gil_scoped_release>(),
             "停止录制\n\n"
             "停止所有录制活动并刷新缓冲区。\n\n"
             "返回:\n"
             "    bool: 成功返回 True,失败返回 False")

        .def("graceful_stop", &RecorderAPI::gracefulStop, py::arg("timeout_ms") = 5000,
             py::call_guard<py::gil_scoped_release>(),
             "优雅停止录制,等待AI分析完成\n\n"
             "发送停止信号并等待分析器处理完所有帧后再关闭。\n\n"
             "参数:\n"
             "    timeout_ms (int): 等待超时时间(毫秒),默认5000ms\n\n"
             "返回:\n"
             "    bool: 成功返回 True,失败返回 False")

        .def("shutdown", &RecorderAPI::shutdown, py::call_guard<py::gil_scoped_release>(),
             "关闭录制器\n\n"
             "释放所有资源,调用后需要重新 initialize() 才能使用。")

        // 状态查询
        .def("get_status", &RecorderAPI::getStatus,
             "获取当前录制状态\n\n"
             "返回:\n"
             "    RecordingStatus: 当前状态枚举值")

        .def("get_stats", &RecorderAPI::getStats,
             "获取录制统计信息\n\n"
             "返回:\n"
             "    RecordingStats: 包含帧数、文件大小等统计数据")

        .def("get_last_error", &RecorderAPI::getLastError,
             "获取最后一次错误信息\n\n"
             "返回:\n"
             "    str: 错误描述字符串")

        // 回调设置 (带 GIL 管理)
        .def(
            "set_status_callback",
            [](RecorderAPI& api, py::function callback) {
                auto safeCallback = make_gil_safe_callback(std::move(callback));
                api.setStatusCallback([safeCallback](RecordingStatus status) {
                    if (Py_IsInitialized()) {
                        py::gil_scoped_acquire gil;
                        try { (*safeCallback)(status); } catch (const py::error_already_set&) { PyErr_Clear(); }
                    }
                });
            },
            py::arg("callback"),
            "设置状态变更回调\n\n"
            "回调签名: callback(status: RecordingStatus) -> None\n\n"
            "示例:\n"
            "    def on_status_change(status):\n"
            "        print(f'状态变更: {status}')\n"
            "    api.set_status_callback(on_status_change)")

        .def(
            "set_error_callback",
            [](RecorderAPI& api, py::function callback) {
                auto safeCallback = make_gil_safe_callback(std::move(callback));
                api.setErrorCallback([safeCallback](const std::string& error) {
                    if (Py_IsInitialized()) {
                        py::gil_scoped_acquire gil;
                        try { (*safeCallback)(error); } catch (const py::error_already_set&) { PyErr_Clear(); }
                    }
                });
            },
            py::arg("callback"),
            "设置错误回调\n\n"
            "回调签名: callback(error_message: str) -> None\n\n"
            "示例:\n"
            "    def on_error(error_msg):\n"
            "        print(f'错误: {error_msg}')\n"
            "    api.set_error_callback(on_error)")

        // 录制模式设置
        .def("set_recording_mode", &RecorderAPI::setRecordingMode, py::arg("mode"),
             py::call_guard<py::gil_scoped_release>(),
             "设置录制模式\n\n"
             "参数:\n"
             "    mode (RecorderMode): VIDEO 或 SNAPSHOT\n\n"
             "示例:\n"
             "    api.set_recording_mode(RecorderMode.SNAPSHOT)  # 单帧模式\n"
             "    api.set_recording_mode(RecorderMode.VIDEO)     # 视频模式")

        .def("get_recording_mode", &RecorderAPI::getRecordingMode,
             "获取当前录制模式\n\n"
             "返回:\n"
             "    RecorderMode: 当前的录制模式")

        // Python 属性风格访问
        .def_property_readonly("status", &RecorderAPI::getStatus, "当前录制状态 (只读属性)")
        .def_property_readonly("stats", &RecorderAPI::getStats, "录制统计信息 (只读属性)")
        .def_property_readonly("last_error", &RecorderAPI::getLastError, "最后错误信息 (只读属性)")

        // 便捷属性
        .def_property_readonly(
            "frame_count", [](const RecorderAPI& api) { return api.getStats().frame_count; },
            "已捕获的帧数 (只读属性)")
        .def_property_readonly(
            "encoded_count", [](const RecorderAPI& api) { return api.getStats().encoded_count; },
            "已编码的帧数 (只读属性)")
        .def_property_readonly(
            "current_fps", [](const RecorderAPI& api) { return api.getStats().current_fps; },
            "当前帧率 (只读属性)")
        .def_property_readonly(
            "is_recording",
            [](const RecorderAPI& api) { return api.getStatus() == RecordingStatus::RECORDING; },
            "是否正在录制 (只读属性)")

        // 上下文管理器支持
        .def("__enter__", [](RecorderAPI& api) -> RecorderAPI& { return api; })
        .def("__exit__",
             [](RecorderAPI& api, py::object, py::object, py::object) {
                 if (api.getStatus() == RecordingStatus::RECORDING ||
                     api.getStatus() == RecordingStatus::PAUSED) {
                     py::gil_scoped_release release;
                     api.stop();
                 }
                 api.shutdown();
                 return false;
             })

        // 字符串表示
        .def("__repr__",
             [](const RecorderAPI& api) {
                 auto stats = api.getStats();
                 auto status = static_cast<int>(api.getStatus());
                 return "<RecorderAPI status=" + std::to_string(status) +
                        " frames=" + std::to_string(stats.frame_count) +
                        " fps=" + std::to_string(stats.current_fps) + ">";
             })

        .def("__str__", [](const RecorderAPI& api) {
            auto stats = api.getStats();
            return "RecorderAPI(status=" + std::string([](RecordingStatus s) {
                       switch (s) {
                           case RecordingStatus::IDLE:
                               return "IDLE";
                           case RecordingStatus::INITIALIZING:
                               return "INITIALIZING";
                           case RecordingStatus::RECORDING:
                               return "RECORDING";
                           case RecordingStatus::PAUSED:
                               return "PAUSED";
                           case RecordingStatus::STOPPING:
                               return "STOPPING";
                           case RecordingStatus::ERROR:
                               return "ERROR";
                           default:
                               return "UNKNOWN";
                       }
                   }(api.getStatus())) +
                   ", frames=" + std::to_string(stats.frame_count) +
                   ", fps=" + std::to_string(stats.current_fps) + ")";
        });
}

}  // namespace Recorder::Bindings
