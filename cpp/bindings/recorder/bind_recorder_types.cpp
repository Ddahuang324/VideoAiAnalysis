/**
 * @file bind_recorder_types.cpp
 * @brief RecordingStatus 枚举和 RecordingStats 结构体的 Python 绑定
 */

#include <pybind11/attr.h>
#include <pybind11/pybind11.h>
#include <pybind11/pytypes.h>
#include <pybind11/stl.h>

#include <string>

#include "Process/Recorder/RecorderAPI.h"

namespace py = pybind11;

namespace Recorder::Bindings {

void bind_recorder_types(py::module& m) {
    using namespace Recorder;

    // 绑定 RecordingStatus 枚举
    py::enum_<RecordingStatus>(m, "RecordingStatus", py::arithmetic(),
                               "录制状态枚举\n\n"
                               "表示录制器的当前运行状态。")
        .value("IDLE", RecordingStatus::IDLE, "空闲状态 - 未初始化或已关闭")
        .value("INITIALIZING", RecordingStatus::INITIALIZING, "初始化中 - 正在加载资源")
        .value("RECORDING", RecordingStatus::RECORDING, "录制中 - 正在捕获和编码帧")
        .value("PAUSED", RecordingStatus::PAUSED, "已暂停 - 暂停捕获但保持资源")
        .value("STOPPING", RecordingStatus::STOPPING, "停止中 - 正在刷新缓冲区")
        .value("ERROR", RecordingStatus::ERROR, "错误状态 - 发生不可恢复错误")
        .export_values()
        .def("__str__",
             [](RecordingStatus status) {
                 switch (status) {
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
             })
        .def("__repr__", [](RecordingStatus status) {
            return "<RecordingStatus." + std::string([](RecordingStatus s) {
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
                   }(status)) +
                   ">";
        });

    // 绑定 RecordingStats 结构体
    py::class_<RecordingStats>(m, "RecordingStats", "录制统计信息")
        .def(py::init<>(), "默认构造函数")
        .def_readwrite("frame_count", &RecordingStats::frame_count, "已捕获的总帧数")
        .def_readwrite("encoded_count", &RecordingStats::encoded_count, "已编码的帧数")
        .def_readwrite("dropped_count", &RecordingStats::dropped_count, "丢弃的帧数")
        .def_readwrite("file_size_bytes", &RecordingStats::file_size_bytes, "输出文件大小 (字节)")
        .def_readwrite("current_fps", &RecordingStats::current_fps, "当前实时帧率")
        .def_readwrite("duration_seconds", &RecordingStats::duration_seconds, "录制时长 (秒)")
        .def("__repr__",
             [](const RecordingStats& stats) {
                 return "<RecordingStats frames=" + std::to_string(stats.frame_count) +
                        " encoded=" + std::to_string(stats.encoded_count) +
                        " dropped=" + std::to_string(stats.dropped_count) +
                        " fps=" + std::to_string(stats.current_fps) +
                        " duration=" + std::to_string(stats.duration_seconds) + "s>";
             })
        // 添加便捷方法
        .def(
            "to_dict",
            [](const RecordingStats& stats) {
                py::dict dict;
                dict["frame_count"] = stats.frame_count;
                dict["encoded_count"] = stats.encoded_count;
                dict["dropped_count"] = stats.dropped_count;
                dict["file_size_bytes"] = stats.file_size_bytes;
                dict["current_fps"] = stats.current_fps;
                dict["duration_seconds"] = stats.duration_seconds;
                return dict;
            },
            "将统计信息转换为 Python 字典");
}

}  // namespace Recorder::Bindings
