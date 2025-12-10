#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "video_processor.h"


namespace py = pybind11;

/**
 * @brief Python 绑定模块
 *
 * 将 C++ 类和函数暴露给 Python
 * 使用方法:
 *   import video_analysis_cpp
 *   processor = video_analysis_cpp.VideoProcessor()
 *   processor.initialize()
 */
PYBIND11_MODULE(video_analysis_cpp, m) {
    m.doc() = "AI Video Analysis C++ Extension Module";

    // 绑定 VideoProcessor 类
    py::class_<VideoProcessor>(m, "VideoProcessor", "视频处理器类 - 提供高性能的视频帧处理功能")
        .def(py::init<>(), "构造函数")

        .def("initialize", &VideoProcessor::initialize,
             "初始化处理器\n\n"
             "Returns:\n"
             "    bool: 是否初始化成功")

        .def("process_frame", &VideoProcessor::processFrame, py::arg("frame_data"),
             "处理单帧视频\n\n"
             "Args:\n"
             "    frame_data (str): 帧数据\n\n"
             "Returns:\n"
             "    str: 处理结果")

        .def("process_frames", &VideoProcessor::processFrames, py::arg("frames"),
             "批量处理多帧视频\n\n"
             "Args:\n"
             "    frames (list[str]): 帧数据列表\n\n"
             "Returns:\n"
             "    list[str]: 处理结果列表")

        .def("get_info", &VideoProcessor::getInfo,
             "获取处理器信息\n\n"
             "Returns:\n"
             "    str: 信息字符串")

        .def("set_parameter", &VideoProcessor::setParameter, py::arg("key"), py::arg("value"),
             "设置处理参数\n\n"
             "Args:\n"
             "    key (str): 参数名\n"
             "    value (float): 参数值")

        .def("get_parameter", &VideoProcessor::getParameter, py::arg("key"),
             "获取处理参数\n\n"
             "Args:\n"
             "    key (str): 参数名\n\n"
             "Returns:\n"
             "    float: 参数值");

    // 添加模块级别的函数
    m.def("version", []() { return "1.0.0"; }, "获取模块版本号");

    m.def("hello", []() { return "Hello from C++ extension!"; }, "测试函数");

    // 添加常量
    m.attr("__version__") = "1.0.0";
    m.attr("__author__") = "AI Video Analysis Team";
}
