cmake_minimum_required(VERSION 3.20)

# ==================== C++ 核心库 ====================
set(CORE_HEADERS
    include/core/ScreenRecorder/ScreenRecorder.h
    include/core/ScreenRecorder/CaptureLayer/VideoGrabber/VideoGrabber.h
    include/core/ScreenRecorder/CaptureLayer/VideoGrabber/VideoGrabberFactory.h
    include/core/ScreenRecorder/CaptureLayer/AudioGrabber/AudioGrabber.h
    include/core/ScreenRecorder/CaptureLayer/AudioGrabber/AudioData.h
    include/core/ScreenRecorder/CaptureLayer/AudioGrabber/WasapiAudioGrabber.h
    include/core/ScreenRecorder/ProcessLayer/FFmpegWrapper.h
    include/core/ScreenRecorder/ProcessLayer/FrameEncoder.h
    include/core/ScreenRecorder/ProcessLayer/AudioEncoder.h
    include/core/ScreenRecorder/ProcessLayer/ThreadSafetyQueue.h
    include/Infra/Log.h
    # KeyFrame 模块头文件
    include/core/KeyFrame/Detectors/MotionDetector.h
    include/core/KeyFrame/Detectors/SceneChangeDetector.h
    include/core/KeyFrame/Detectors/TextDetector.h
    include/core/KeyFrame/Foundation/DataConverter.h
    include/core/KeyFrame/Foundation/ModelManager.h
    include/core/KeyFrame/Foundation/ONNXSession.h
    include/core/KeyFrame/Foundation/TensorBuffer.h
)

add_library(ai_video_core STATIC
    ${CORE_HEADERS}
    src/core/ScreenRecorder.cpp
    src/core/ScreenRecorder/CaptureLayer/VideoGrabber/VideoGrabberFactory.cpp
    src/core/ScreenRecorder/CaptureLayer/VideoGrabber/Win/GDI_Grabber.cpp
    src/core/ScreenRecorder/CaptureLayer/VideoGrabber/Win/DXGI_Grabber.cpp
    src/core/ScreenRecorder/CaptureLayer/AudioGrabber/WasapiAudioGrabber.cpp
    src/core/ScreenRecorder/ProcessLayer/FFmpegWrapper.cpp
    src/core/ScreenRecorder/ProcessLayer/FrameEncoder.cpp
    src/core/ScreenRecorder/ProcessLayer/AudioEncoder.cpp
    src/core/ScreenRecorder/ProcessLayer/FrameGrabberThread.cpp
    # KeyFrame 模块源文件
    src/core/KeyFrame/Detectors/MotionDetector.cpp
    src/core/KeyFrame/Detectors/SceneChangeDetector.cpp
    src/core/KeyFrame/Detectors/TextDetector.cpp
    src/core/KeyFrame/Foundation/DataConverter.cpp
    src/core/KeyFrame/Foundation/ModelManager.cpp
    src/core/KeyFrame/Foundation/ONNXSession.cpp
    src/core/KeyFrame/Foundation/TensorBuffer.cpp
)

target_include_directories(ai_video_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    # ScreenRecorder 核心头文件路径
    ${CMAKE_CURRENT_SOURCE_DIR}/include/core/ScreenRecorder
    # 让只写文件名即可找到核心抓屏接口与具体实现头文件
    ${CMAKE_CURRENT_SOURCE_DIR}/include/core/ScreenRecorder/CaptureLayer/VideoGrabber
    ${CMAKE_CURRENT_SOURCE_DIR}/include/core/ScreenRecorder/CaptureLayer/VideoGrabber/Win
    ${CMAKE_CURRENT_SOURCE_DIR}/include/core/ScreenRecorder/CaptureLayer/AudioGrabber
    # ProcessLayer 头文件路径，支持仅文件名包含
    ${CMAKE_CURRENT_SOURCE_DIR}/include/core/ScreenRecorder/ProcessLayer
    # Test 文件夹路径，支持基准测试头文件
    ${CMAKE_CURRENT_SOURCE_DIR}/include/core/ScreenRecorder/ProcessLayer/Test
    # Infra 层头文件路径，支持 Log.h 等基础设施头文件
    ${CMAKE_CURRENT_SOURCE_DIR}/include/Infra
    # KeyFrame 模块头文件路径
    ${CMAKE_CURRENT_SOURCE_DIR}/include/core/KeyFrame
    ${CMAKE_CURRENT_SOURCE_DIR}/include/core/KeyFrame/Detectors
    ${CMAKE_CURRENT_SOURCE_DIR}/include/core/KeyFrame/Foundation
)

# 链接 FFmpeg
if(FFmpeg_FOUND)
    target_include_directories(ai_video_core PUBLIC ${FFmpeg_INCLUDE_DIR})
    target_link_libraries(ai_video_core PUBLIC
        ${FFmpeg_avutil_LIBRARY}
        ${FFmpeg_avformat_LIBRARY}
        ${FFmpeg_avcodec_LIBRARY}
        ${FFmpeg_swscale_LIBRARY}
        ${FFmpeg_swresample_LIBRARY}
    )
    message(STATUS "✓ Linked FFmpeg to ai_video_core")
endif()

# 链接 ONNX Runtime
if(ONNXRUNTIME_FOUND)
    target_include_directories(ai_video_core PUBLIC ${ONNXRUNTIME_INCLUDE_DIR})
    target_link_libraries(ai_video_core PUBLIC ${ONNXRUNTIME_LIBRARY})
    
    # Windows: 拷贝 ONNX Runtime DLLs 到输出目录
    if(WIN32 AND ONNXRUNTIME_DLLS)
        foreach(dll ${ONNXRUNTIME_DLLS})
            add_custom_command(TARGET ai_video_core POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "${dll}"
                    "$<TARGET_FILE_DIR:ai_video_core>"
                COMMENT "Copying ONNX Runtime DLL: ${dll}"
            )
        endforeach()
    endif()
    
    message(STATUS "✓ Linked ONNX Runtime to ai_video_core")
endif()

# 链接 OpenCV
if(OpenCV_FOUND)
    target_include_directories(ai_video_core PUBLIC ${OpenCV_INCLUDE_DIRS})
    target_link_libraries(ai_video_core PUBLIC ${OpenCV_LIBS})
    
    # Windows: 拷贝 OpenCV DLLs 到输出目录
    if(WIN32 AND OpenCV_DLLS)
        foreach(dll ${OpenCV_DLLS})
            add_custom_command(TARGET ai_video_core POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "${dll}"
                    "$<TARGET_FILE_DIR:ai_video_core>"
                COMMENT "Copying OpenCV DLL: ${dll}"
            )
        endforeach()
    endif()
    
    message(STATUS "✓ Linked OpenCV to ai_video_core")
endif()

target_compile_features(ai_video_core PUBLIC cxx_std_17)

# ==================== Python 绑定模块 ====================
add_subdirectory(bindings)
