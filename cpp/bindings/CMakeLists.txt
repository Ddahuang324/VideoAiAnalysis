cmake_minimum_required(VERSION 3.20)

# ==================== Python 绑定模块 ====================
if(pybind11_FOUND)
    pybind11_add_module(video_analysis_cpp
        bindings.cpp
    )
    
    target_include_directories(video_analysis_cpp PRIVATE
        ${CMAKE_SOURCE_DIR}/cpp/include
    )
    
    target_compile_features(video_analysis_cpp PRIVATE cxx_std_17)
    
    # 设置输出目录
    set_target_properties(video_analysis_cpp PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/python
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/python
    )
    
    message(STATUS "Python bindings module configured: video_analysis_cpp")
else()
    message(WARNING "pybind11 not found, skipping Python bindings")
endif()
