cmake_minimum_required(VERSION 3.20)

# ==================== Python 绑定模块 ====================
if(pybind11_FOUND)
    # 添加所有绑定源文件
    pybind11_add_module(video_analysis_cpp
        bindings.cpp
        bind_frame_grabber.cpp
        # 未来可以添加更多绑定文件
        # bind_screen_recorder.cpp
        # bind_video_processor.cpp
    )
    
    target_include_directories(video_analysis_cpp PRIVATE
        ${CMAKE_SOURCE_DIR}/cpp/include
        ${CMAKE_SOURCE_DIR}/cpp/include/core/ScreenRecorder/CaptureLayer
        ${CMAKE_SOURCE_DIR}/cpp/include/core/ScreenRecorder/CaptureLayer/SpecificGrabber/Win
        ${CMAKE_SOURCE_DIR}/cpp/include/core/ScreenRecorder/ProcessLayer
        ${CMAKE_SOURCE_DIR}/cpp/include/Infra
    )
    
    # 链接核心库
    target_link_libraries(video_analysis_cpp PRIVATE
        ai_video_core
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
