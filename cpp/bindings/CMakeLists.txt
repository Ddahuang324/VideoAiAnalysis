cmake_minimum_required(VERSION 3.20)

# ==================== Python 绑定模块 ====================
if(pybind11_FOUND)
    # 添加所有绑定源文件
    pybind11_add_module(Video_Recording_Moudle
        bindings.cpp
        bind_screen_recorder.cpp
        # 未来可以添加更多绑定文件
        # bind_video_processor.cpp
    )
    
    target_include_directories(Video_Recording_Moudle PRIVATE
        ${CMAKE_SOURCE_DIR}/cpp/include
        ${CMAKE_SOURCE_DIR}/cpp/include/core/ScreenRecorder/CaptureLayer/VideoGrabber
        ${CMAKE_SOURCE_DIR}/cpp/include/core/ScreenRecorder/CaptureLayer/VideoGrabber/Win
        ${CMAKE_SOURCE_DIR}/cpp/include/core/ScreenRecorder/CaptureLayer/AudioGrabber
        ${CMAKE_SOURCE_DIR}/cpp/include/core/ScreenRecorder/ProcessLayer
        ${CMAKE_SOURCE_DIR}/cpp/include/Infra
    )
    
    # 链接核心库
    target_link_libraries(Video_Recording_Moudle PRIVATE
        ai_video_core
    )
    
    target_compile_features(Video_Recording_Moudle PRIVATE cxx_std_17)
    
    # 设置输出目录
    set_target_properties(Video_Recording_Moudle PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/python
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/python
    )
    
    # 复制 FFmpeg DLL 到 Python 模块目录 (解决运行时 DLL 加载问题)
    if(FFmpeg_DLLS)
        add_custom_command(TARGET Video_Recording_Moudle POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                ${FFmpeg_DLLS}
                $<TARGET_FILE_DIR:Video_Recording_Moudle>
            COMMENT "Copying FFmpeg DLLs to Python module directory..."
        )
        message(STATUS "✓ FFmpeg DLLs will be copied to: ${CMAKE_BINARY_DIR}/python")
    endif()
    
    message(STATUS "Python bindings module configured: Video_Recording_Moudle")
else()
    message(WARNING "pybind11 not found, skipping Python bindings")
endif()
