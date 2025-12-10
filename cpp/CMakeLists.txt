cmake_minimum_required(VERSION 3.20)

# ==================== C++ 核心库 ====================
add_library(ai_video_core STATIC
    src/video_processor.cpp
)

target_include_directories(ai_video_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_features(ai_video_core PUBLIC cxx_std_17)

# ==================== Python 绑定模块 ====================
add_subdirectory(bindings)
