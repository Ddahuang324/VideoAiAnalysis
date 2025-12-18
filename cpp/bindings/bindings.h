#pragma once

#include <pybind11/pybind11.h>

namespace py = pybind11;

// 声明各个模块的绑定函数
void bind_frame_grabber(py::module& m);
void bind_encoder(py::module& m);
// 未来可以添加更多模块
// void bind_video_processor(py::module& m);
