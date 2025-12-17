cmake_minimum_required(VERSION 3.20)

project(AiVideoAnalysisSystem
    VERSION 1.0.0
    DESCRIPTION "AI Video Analysis System with C++ and Python"
    LANGUAGES CXX
)

# ==================== 编译器设置 ====================
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 导出 compile_commands.json 供 clangd 使用
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# 生成 .qmlls.ini 文件，解决 QML 导入警告
set(QT_QML_GENERATE_QMLLS_INI ON)

# ==================== 输出目录设置 ====================
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# ==================== 编译选项 ====================
# 检测是否使用 clang-cl (MSVC 兼容模式)
if(MSVC AND CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    # clang-cl: 使用 MSVC 风格的编译选项
    message(STATUS "Using clang-cl (MSVC compatible mode)")
    add_compile_options(
        /W3
        /EHsc
    )
    
    if(CMAKE_BUILD_TYPE MATCHES "Debug")
        add_compile_options(/Od /Zi)
    else()
        add_compile_options(/O2)
    endif()
elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    # 普通 clang/gcc: 使用 GNU 风格的编译选项
    message(STATUS "Using Clang/GCC (GNU compatible mode)")
    add_compile_options(
        -Wall
        -Wextra
        -Wpedantic
        -Wno-unused-parameter
        -fcolor-diagnostics
    )
    
    if(CMAKE_BUILD_TYPE MATCHES "Debug")
        add_compile_options(-g -O0)
    else()
        add_compile_options(-O3)
    endif()
endif()

# ==================== 查找依赖 ====================
# 查找 FFmpeg (需要预编译版本，不是源代码)
# 从 https://github.com/BtbN/FFmpeg-Builds/releases 下载预编译版本
set(FFmpeg_ROOT "${CMAKE_SOURCE_DIR}/packages/ffmpeg")
if(EXISTS "${FFmpeg_ROOT}")
    message(STATUS "FFmpeg root: ${FFmpeg_ROOT}")
    # 自定义查找 FFmpeg
    find_path(FFmpeg_INCLUDE_DIR
        NAMES libavutil/avutil.h libavformat/avformat.h libavcodec/avcodec.h
        PATHS "${FFmpeg_ROOT}/include"
        NO_DEFAULT_PATH
    )
    
    find_library(FFmpeg_avutil_LIBRARY
        NAMES avutil avutil-60 libavutil
        PATHS
            "${FFmpeg_ROOT}/lib/x64"
            "${FFmpeg_ROOT}/lib"
            "${FFmpeg_ROOT}/lib64"
        NO_DEFAULT_PATH
    )
    
    find_library(FFmpeg_avformat_LIBRARY
        NAMES avformat avformat-62 libavformat
        PATHS
            "${FFmpeg_ROOT}/lib/x64"
            "${FFmpeg_ROOT}/lib"
            "${FFmpeg_ROOT}/lib64"
        NO_DEFAULT_PATH
    )
    
    find_library(FFmpeg_avcodec_LIBRARY
        NAMES avcodec avcodec-62 libavcodec
        PATHS
            "${FFmpeg_ROOT}/lib/x64"
            "${FFmpeg_ROOT}/lib"
            "${FFmpeg_ROOT}/lib64"
        NO_DEFAULT_PATH
    )
    
    find_library(FFmpeg_swscale_LIBRARY
        NAMES swscale swscale-9 libswscale
        PATHS
            "${FFmpeg_ROOT}/lib/x64"
            "${FFmpeg_ROOT}/lib"
            "${FFmpeg_ROOT}/lib64"
        NO_DEFAULT_PATH
    )
    
    if(FFmpeg_INCLUDE_DIR AND FFmpeg_avutil_LIBRARY AND FFmpeg_avformat_LIBRARY AND FFmpeg_avcodec_LIBRARY)
        set(FFmpeg_FOUND TRUE)
        message(STATUS "✓ FFmpeg found")
        message(STATUS "  Include: ${FFmpeg_INCLUDE_DIR}")
        message(STATUS "  avutil: ${FFmpeg_avutil_LIBRARY}")
        message(STATUS "  avformat: ${FFmpeg_avformat_LIBRARY}")
        message(STATUS "  avcodec: ${FFmpeg_avcodec_LIBRARY}")
        message(STATUS "  swscale: ${FFmpeg_swscale_LIBRARY}")
    else()
        message(WARNING "FFmpeg libraries not found. Please place FFmpeg headers and libraries in ${FFmpeg_ROOT}")
        message(STATUS "  Expected header location: ${FFmpeg_ROOT}/include/libavutil/avutil.h")
        message(STATUS "  Expected lib location: ${FFmpeg_ROOT}/lib/x64/")
    endif()
else()
    message(WARNING "FFmpeg directory not found at ${FFmpeg_ROOT}")
endif()

# 查找 Python
find_package(Python COMPONENTS Interpreter Development REQUIRED)
message(STATUS "Python version: ${Python_VERSION}")
message(STATUS "Python include dirs: ${Python_INCLUDE_DIRS}")
message(STATUS "Python libraries: ${Python_LIBRARIES}")

# 查找 pybind11
# 方法1: 尝试通过 Python 动态获取路径 (最稳健的跨平台方法)
execute_process(
    COMMAND "${Python_EXECUTABLE}" -c "import pybind11; print(pybind11.get_cmake_dir())"
    OUTPUT_VARIABLE _tmp_pybind11_dir
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_QUIET
)

if(_tmp_pybind11_dir)
    message(STATUS "Found pybind11 via Python: ${_tmp_pybind11_dir}")
    set(pybind11_DIR "${_tmp_pybind11_dir}")
endif()

find_package(pybind11 CONFIG)

if(NOT pybind11_FOUND)
    # 方法2: 如果上面的失败，尝试包含目录
    execute_process(
        COMMAND "${Python_EXECUTABLE}" -c "import pybind11; print(pybind11.get_include())"
        OUTPUT_VARIABLE _tmp_pybind11_include
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )
    
    if(_tmp_pybind11_include)
        message(STATUS "Found pybind11 include via Python: ${_tmp_pybind11_include}")
        set(PYBIND11_INCLUDE_DIR "${_tmp_pybind11_include}")
    endif()
endif()

# ==================== 包含目录 ====================
include_directories(
    ${CMAKE_SOURCE_DIR}/cpp/include
    ${Python_INCLUDE_DIRS}
)

# 强制使用虚拟环境的 pybind11 (如果存在)
if(EXISTS "${CMAKE_SOURCE_DIR}/.venv/Lib/site-packages/pybind11/include")
    set(PYBIND11_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/.venv/Lib/site-packages/pybind11/include")
    message(STATUS "Using venv pybind11: ${PYBIND11_INCLUDE_DIR}")
endif()

if(DEFINED PYBIND11_INCLUDE_DIR)
    include_directories(${PYBIND11_INCLUDE_DIR})
endif()

# ==================== C++ 子项目 ====================
add_subdirectory(cpp)

# ==================== 安装规则 ====================
install(TARGETS ai_video_core
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)

# ==================== 测试 ====================
enable_testing()
add_subdirectory(tests)

# ==================== 打印配置信息 ====================
message(STATUS "")
message(STATUS "==================== Build Configuration ====================")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "C++ compiler: ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "C++ standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "Install prefix: ${CMAKE_INSTALL_PREFIX}")
message(STATUS "=============================================================")
message(STATUS "")
