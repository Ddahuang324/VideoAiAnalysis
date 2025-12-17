#include <pybind11/cast.h>
#include <pybind11/functional.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include <memory>

#include "FrameGrabberThread.h"
#include "IScreenGrabber.h"
#include "ThreadSafetyQueue.h"
#include "bindings.h"

void bind_frame_grabber(py::module& m) {  // confused:关于 Pybind绑定的语法
    // 绑定 FrameGrabberThread 类
    py::class_<FrameGrabberThread>(m, "FrameGrabberThread",
                                   "Frame grabbing thread for screen capture")
        .def(py::init<std::shared_ptr<IScreenGrabber>, ThreadSafetyQueue<FrameData>&, int>(),
             py::arg("grabber"), py::arg("queue"), py::arg("target_fps") = 60,
             "Initialize FrameGrabberThread")

        // 基本控制方法
        .def("start", &FrameGrabberThread::start, "Start the capture thread")
        .def("stop", &FrameGrabberThread::stop, "Stop the capture thread")
        .def("pause", &FrameGrabberThread::pause, "Pause capturing")
        .def("resume", &FrameGrabberThread::resume, "Resume capturing")

        // 状态查询方法
        .def("is_running", &FrameGrabberThread::isRunning, "Check if thread is running")
        .def("is_paused", &FrameGrabberThread::isPaused, "Check if capturing is paused")

        // 统计数据获取
        .def("get_captured_frame_count", &FrameGrabberThread::getCapturedFrameCount,
             "Get the number of captured frames")
        .def("get_dropped_frame_count", &FrameGrabberThread::getDroppedFrameCount,
             "Get the number of dropped frames")
        .def("get_current_fps", &FrameGrabberThread::getCuttentFps,
             "Get current capture frame rate")

        // 回调函数设置
        .def("set_progress_callback", &FrameGrabberThread::setProgressCallback, py::arg("callback"),
             "Set progress callback: callback(captured_count, queue_size, fps)")
        .def("set_error_callback", &FrameGrabberThread::setErrorCallback, py::arg("callback"),
             "Set error callback: callback(error_message)")
        .def("set_dropped_callback", &FrameGrabberThread::setDroppedCallback, py::arg("callback"),
             "Set dropped frames callback: callback(dropped_count)");

    // 可以在这里添加相关的辅助类或函数
}
