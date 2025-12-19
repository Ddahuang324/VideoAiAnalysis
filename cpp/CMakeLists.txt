cmake_minimum_required(VERSION 3.20)

# ==================== C++ 核心库 ====================
add_library(ai_video_core STATIC
    src/core/ScreenRecorder.cpp
    src/core/ScreenRecorder/CaptureLayer/GrabberFactory.cpp
    src/core/ScreenRecorder/CaptureLayer/SpecificGrabber/Win/GDI_Grabber.cpp
    src/core/ScreenRecorder/CaptureLayer/SpecificGrabber/Win/DXGI_Grabber.cpp
    src/core/ScreenRecorder/ProcessLayer/FFmpegWrapper.cpp
    src/core/ScreenRecorder/ProcessLayer/FrameEncoder.cpp
    src/core/ScreenRecorder/ProcessLayer/FrameGrabberThread.cpp
)

target_include_directories(ai_video_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    # ScreenRecorder 核心头文件路径
    ${CMAKE_CURRENT_SOURCE_DIR}/include/core/ScreenRecorder
    # 让只写文件名即可找到核心抓屏接口与具体实现头文件
    ${CMAKE_CURRENT_SOURCE_DIR}/include/core/ScreenRecorder/CaptureLayer
    ${CMAKE_CURRENT_SOURCE_DIR}/include/core/ScreenRecorder/CaptureLayer/SpecificGrabber/Win
    # ProcessLayer 头文件路径，支持仅文件名包含
    ${CMAKE_CURRENT_SOURCE_DIR}/include/core/ScreenRecorder/ProcessLayer
    # Test 文件夹路径，支持基准测试头文件
    ${CMAKE_CURRENT_SOURCE_DIR}/include/core/ScreenRecorder/ProcessLayer/Test
    # Infra 层头文件路径，支持 Log.h 等基础设施头文件
    ${CMAKE_CURRENT_SOURCE_DIR}/include/Infra
)

# 链接 FFmpeg
if(FFmpeg_FOUND)
    target_include_directories(ai_video_core PUBLIC ${FFmpeg_INCLUDE_DIR})
    target_link_libraries(ai_video_core PUBLIC
        ${FFmpeg_avutil_LIBRARY}
        ${FFmpeg_avformat_LIBRARY}
        ${FFmpeg_avcodec_LIBRARY}
        ${FFmpeg_swscale_LIBRARY}
    )
    message(STATUS "✓ Linked FFmpeg to ai_video_core")
endif()

target_compile_features(ai_video_core PUBLIC cxx_std_17)

# ==================== Python 绑定模块 ====================
add_subdirectory(bindings)
