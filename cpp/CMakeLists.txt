cmake_minimum_required(VERSION 3.20)

# ==================== C++ 核心库 ====================
add_library(ai_video_core STATIC
    src/core/ScreenRecorder/CaptureLayer/GrabberFactory.cpp
)

target_include_directories(ai_video_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    # 让只写文件名即可找到核心抓屏接口与具体实现头文件
    ${CMAKE_CURRENT_SOURCE_DIR}/include/core/ScreenRecorder/CaptureLayer
    ${CMAKE_CURRENT_SOURCE_DIR}/include/core/ScreenRecorder/CaptureLayer/SpecificGrabber/Win
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
