# ==================== Analyzer Module ====================

# 创建 Python 模块
pybind11_add_module(analyzer_module
    analyzer_module.cpp
    bind_analyzer_types.cpp
    bind_analyzer_config.cpp
    bind_analyzer_api.cpp
)

# 包含目录
target_include_directories(analyzer_module PRIVATE
    ${CMAKE_SOURCE_DIR}/cpp/include
    ${CMAKE_SOURCE_DIR}/cpp/include/core/ScreenRecorder/CaptureLayer/VideoGrabber
    ${CMAKE_SOURCE_DIR}/cpp/include/core/ScreenRecorder/CaptureLayer/VideoGrabber/Win
    ${CMAKE_SOURCE_DIR}/cpp/include/core/ScreenRecorder/CaptureLayer/AudioGrabber
    ${CMAKE_SOURCE_DIR}/cpp/include/core/ScreenRecorder/ProcessLayer
    ${CMAKE_SOURCE_DIR}/cpp/include/Infra
    ${CMAKE_SOURCE_DIR}/cpp/include/core/KeyFrame
    ${CMAKE_SOURCE_DIR}/cpp/include/core/KeyFrame/Detectors
    ${CMAKE_SOURCE_DIR}/cpp/include/core/KeyFrame/Foundation
    ${CMAKE_SOURCE_DIR}/cpp/include/core/KeyFrame/FrameAnalyzer
    ${CMAKE_SOURCE_DIR}/cpp/include/core/MQInfra
    ${CMAKE_SOURCE_DIR}/cpp/include/Process/Analyzer
    ${CMAKE_SOURCE_DIR}/cpp/include/Process/IPC
    ${CMAKE_SOURCE_DIR}/cpp/include/core/Config
)

# 链接核心库
target_link_libraries(analyzer_module PRIVATE
    ai_video_core
)

if(TARGET nlohmann_json::nlohmann_json)
    target_link_libraries(analyzer_module PRIVATE nlohmann_json::nlohmann_json)
elseif(TARGET nlohmann_json)
    target_link_libraries(analyzer_module PRIVATE nlohmann_json)
endif()

# C++ 标准
target_compile_features(analyzer_module PRIVATE cxx_std_17)

# 设置输出目录
set_target_properties(analyzer_module PROPERTIES
    OUTPUT_NAME "analyzer_module"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/python"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/python"
)

# 复制 ONNX Runtime DLL 到 Python 模块目录
if(WIN32 AND ONNXRUNTIME_DLLS)
    add_custom_command(TARGET analyzer_module POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${ONNXRUNTIME_DLLS}
            $<TARGET_FILE_DIR:analyzer_module>
        COMMENT "Copying ONNX Runtime DLLs to analyzer_module directory..."
    )
endif()

# 复制 OpenCV DLL 到 Python 模块目录
if(WIN32 AND OpenCV_DLLS)
    add_custom_command(TARGET analyzer_module POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${OpenCV_DLLS}
            $<TARGET_FILE_DIR:analyzer_module>
        COMMENT "Copying OpenCV DLLs to analyzer_module directory..."
    )
endif()

# Windows: 设置 .pyd 扩展名
if(WIN32)
    set_target_properties(analyzer_module PROPERTIES
        SUFFIX ".pyd"
    )
endif()

message(STATUS "Analyzer module configured")
