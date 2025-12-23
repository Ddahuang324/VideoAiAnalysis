#include "bindings.h"

#include <pybind11/detail/common.h>
#include <pybind11/functional.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

// 主模块入口点 - 只能有一个 PYBIND11_MODULE
PYBIND11_MODULE(Video_Recording_Moudle, m) {
    m.doc() = "AI Video Analysis System - C++ Core Module";

    // 注册各个子模块的绑定
    bind_Screen_Recorder(m);
}
