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
    include/core/KeyFrame/Detectors/MotionVisualizer.h
    include/core/KeyFrame/Detectors/SceneChangeDetector.h
    include/core/KeyFrame/Detectors/TextDetector.h
    include/core/KeyFrame/Foundation/DataConverter.h
    include/core/KeyFrame/Foundation/ModelManager.h
    include/core/KeyFrame/Foundation/ONNXSession.h
    include/core/KeyFrame/Foundation/TensorBuffer.h
    include/core/KeyFrame/FrameAnalyzer/DynamicCalculator.h
    include/core/KeyFrame/FrameAnalyzer/FrameScorer.h
    include/core/KeyFrame/FrameAnalyzer/IFrameAnalyzer.h
    include/core/KeyFrame/FrameAnalyzer/KeyFrameDetector.h
    include/core/KeyFrame/FrameAnalyzer/StandardFrameAnalyzer.h
    include/core/MQInfra/FramePublisher.h
    include/core/MQInfra/FrameSubscriber.h
    include/core/MQInfra/KeyFrameMetaDataPublisher.h
    include/core/MQInfra/KeyFrameMetaDataSubscriber.h
    include/core/MQInfra/Protocol.h
    include/core/KeyFrame/FrameAnalyzer/FrameAnalyzer.h
    include/core/KeyFrame/FrameAnalyzer/FrameResource.h
    include/core/KeyFrame/KeyFrameVideoEncoder.h
    include/core/ScreenRecorder/ProcessLayer/FrameGrabberThread.h
    include/core/ScreenRecorder/CaptureLayer/VideoGrabber/Linux/PipeWire_Grabber.h
    include/core/ScreenRecorder/CaptureLayer/VideoGrabber/Linux/X11_Grabber.h
    include/core/ScreenRecorder/CaptureLayer/VideoGrabber/Win/DXGI_Grabber.h
    include/core/ScreenRecorder/CaptureLayer/VideoGrabber/Win/GDI_Grabber.h
    include/core/ScreenRecorder/RingFrameBuffer.h
    include/core/KeyFrame/KeyFrameAnalyzerService.h
    include/Process/Analyzer/AnalyzerAPI.h
    include/Process/Recorder/RecorderAPI.h
    include/Process/IPC/AnalyzerProcessClient.h
    include/Process/IPC/CommandProtocol.h
    include/Process/IPC/IIPCClientBase.h
    include/Process/IPC/IPCServer.h
    include/Process/IPC/RecorderProcessClient.h
    include/core/Config/ConfigBase.h
    include/core/Config/ConfigValidation.h
    include/core/Config/UnifiedConfig.h
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
    src/core/KeyFrame/Detectors/MotionVisualizer.cpp
    src/core/KeyFrame/Detectors/SceneChangeDetector.cpp
    src/core/KeyFrame/Detectors/TextDetector.cpp
    src/core/KeyFrame/Foundation/DataConverter.cpp
    src/core/KeyFrame/Foundation/ModelManager.cpp
    src/core/KeyFrame/Foundation/ONNXSession.cpp
    src/core/KeyFrame/Foundation/TensorBuffer.cpp
    src/core/KeyFrame/FrameAnalyzer/DynamicCalculator.cpp
    src/core/KeyFrame/FrameAnalyzer/FrameScorer.cpp
    src/core/KeyFrame/FrameAnalyzer/KeyFrameDetector.cpp
    src/core/KeyFrame/FrameAnalyzer/StandardFrameAnalyzer.cpp
    src/Infra/Log.cpp
    src/core/MQInfra/FramePublisher.cpp
    src/core/MQInfra/FrameSubscriber.cpp
    src/core/MQInfra/KeyFrameMetaDataPublisher.cpp
    src/core/MQInfra/KeyFrameMetaDataSubscriber.cpp
    src/core/MQInfra/Protocol.cpp
    src/core/ScreenRecorder/ScreenRecorder.cpp
    src/core/KeyFrame/FrameAnalyzer/FrameAnalyzer.cpp
    src/core/KeyFrame/FrameAnalyzer/FrameResource.cpp
    src/core/KeyFrame/FrameAnalyzer/IFrameAnalyzer.cpp
    src/core/ScreenRecorder/ProcessLayer/ThreadSafetyQueue.cpp
    src/core/ScreenRecorder/CaptureLayer/AudioGrabber/AudioData.cpp
    src/core/ScreenRecorder/CaptureLayer/AudioGrabber/AudioGrabber.cpp
    src/core/ScreenRecorder/CaptureLayer/VideoGrabber/VideoGrabber.cpp
    src/core/ScreenRecorder/CaptureLayer/VideoGrabber/Linux/PipeWire_Grabber.cpp
    src/core/ScreenRecorder/CaptureLayer/VideoGrabber/Linux/X11_Grabber.cpp
    src/core/ScreenRecorder/RingFrameBuffer.cpp
    src/core/KeyFrame/KeyFrameAnalyzerService.cpp
    src/core/KeyFrame/KeyFrameVideoEncoder.cpp
    src/Process/Analyzer/AnalyzerAPI.cpp
    src/Process/Recorder/RecorderAPI.cpp
    src/Process/IPC/AnalyzerProcessClient.cpp
    src/Process/IPC/CommandProtocol.cpp
    src/Process/IPC/IIPCClientBase.cpp
    src/Process/IPC/IPCServer.cpp
    src/Process/IPC/RecorderProcessClient.cpp
    src/core/Config/ConfigBase.cpp
    src/core/Config/ConfigValidation.cpp
    src/core/Config/UnifiedConfig.cpp
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
    ${CMAKE_CURRENT_SOURCE_DIR}/include/core/KeyFrame/FrameAnalyzer
    # MQInfra 模块头文件路径，支持仅文件名包含 Protocol.h 等
    ${CMAKE_CURRENT_SOURCE_DIR}/include/core/MQInfra
    ${CMAKE_CURRENT_SOURCE_DIR}/include/core/ScreenRecorder/CaptureLayer/VideoGrabber/Linux
    ${CMAKE_CURRENT_SOURCE_DIR}/include/Process/Analyzer
    ${CMAKE_CURRENT_SOURCE_DIR}/include/Process/Recorder
    ${CMAKE_CURRENT_SOURCE_DIR}/include/Process/IPC
    ${CMAKE_CURRENT_SOURCE_DIR}/include/core/Config
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

# ==================== ZeroMQ 链接（如果通过根 CMake 拉取） ====================
if(TARGET cppzmq::cppzmq OR TARGET cppzmq OR TARGET libzmq::libzmq OR TARGET libzmq)
    message(STATUS "ZeroMQ targets detected; linking to ai_video_core if available")
    # 尝试以规范的命名空间链接（优先使用 namespaced alias）
    if(TARGET cppzmq::cppzmq)
        target_link_libraries(ai_video_core PUBLIC cppzmq::cppzmq)
    elseif(TARGET cppzmq)
        target_link_libraries(ai_video_core PUBLIC cppzmq)
    endif()

    if(TARGET libzmq::libzmq)
        target_link_libraries(ai_video_core PUBLIC libzmq::libzmq)
    elseif(TARGET libzmq)
        target_link_libraries(ai_video_core PUBLIC libzmq)
    endif()

    message(STATUS "✓ Linked ZeroMQ (if targets provided by FetchContent)")
endif()

# Link nlohmann/json if available
if(TARGET nlohmann_json::nlohmann_json OR TARGET nlohmann_json)
    message(STATUS "Linking nlohmann/json to ai_video_core")
    if(TARGET nlohmann_json::nlohmann_json)
        target_link_libraries(ai_video_core PUBLIC nlohmann_json::nlohmann_json)
    else()
        target_link_libraries(ai_video_core PUBLIC nlohmann_json)
    endif()
    message(STATUS "✓ Linked nlohmann/json to ai_video_core")
endif()

target_compile_features(ai_video_core PUBLIC cxx_std_17)

# ==================== 可执行文件构建 (Process 模式) ====================

# 设置可执行文件输出目录
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# 1. RecorderProcess
add_executable(RecorderProcess
    src/Process/Recorder/RecorderProcessMain.cpp
)

target_link_libraries(RecorderProcess PRIVATE ai_video_core)
target_compile_features(RecorderProcess PRIVATE cxx_std_17)

# 2. AnalyzerProcess
add_executable(AnalyzerProcess
    src/Process/Analyzer/AnaylerProcessMain.cpp
)

target_link_libraries(AnalyzerProcess PRIVATE ai_video_core)
target_compile_features(AnalyzerProcess PRIVATE cxx_std_17)

# Windows 平台：拷贝依赖的 DLL 到可执行文件目录
if(WIN32)
    # 定义需要拷贝到 bin 目录的 DLL 列表
    set(DEPENDENT_DLLS "")
    if(ONNXRUNTIME_DLLS)
        list(APPEND DEPENDENT_DLLS ${ONNXRUNTIME_DLLS})
    endif()
    if(OpenCV_DLLS)
        list(APPEND DEPENDENT_DLLS ${OpenCV_DLLS})
    endif()

    # 为每个可执行文件添加 POST_BUILD 拷贝逻辑
    foreach(exe_target RecorderProcess AnalyzerProcess)
        foreach(dll ${DEPENDENT_DLLS})
            add_custom_command(TARGET ${exe_target} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "${dll}"
                    "$<TARGET_FILE_DIR:${exe_target}>"
                COMMENT "Copying DLL to ${exe_target} directory: ${dll}"
            )
        endforeach()
    endforeach()
endif()

message(STATUS "✓ Added executable targets: RecorderProcess, AnalyzerProcess")

# ==================== Python 绑定模块 ====================
add_subdirectory(bindings)
