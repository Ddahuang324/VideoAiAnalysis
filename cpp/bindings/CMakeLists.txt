cmake_minimum_required(VERSION 3.20)

# ==================== Python 绑定模块 ====================

# 查找 pybind11
if(NOT TARGET pybind11::module)
    find_package(pybind11 REQUIRED)
endif()

# ==================== Recorder Module ====================
add_subdirectory(recorder)

# ==================== Analyzer Module ====================
add_subdirectory(analyzer)

# ==================== Meta Target ====================
add_custom_target(python_bindings
    DEPENDS recorder_module analyzer_module
)

message(STATUS "Python bindings configured: recorder_module, analyzer_module")
