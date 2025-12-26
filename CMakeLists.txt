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
# ==================== FFmpeg (FetchContent Prebuilt) ====================
include(FetchContent)

# 设置 FetchContent 策略以避免警告
if(POLICY CMP0169)
    cmake_policy(SET CMP0169 OLD)  # 允许使用 FetchContent_Populate 进行手动配置
endif()

FetchContent_Declare(
    ffmpeg_prebuilt
    URL https://github.com/BtbN/FFmpeg-Builds/releases/download/latest/ffmpeg-master-latest-win64-gpl-shared.zip
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE  # 使用提取时间戳而不是归档时间戳
)

if(NOT ffmpeg_prebuilt_POPULATED)
    message(STATUS "Downloading/Extracting FFmpeg prebuilt binaries...")
    FetchContent_Populate(ffmpeg_prebuilt)
    
    set(FFmpeg_ROOT "${ffmpeg_prebuilt_SOURCE_DIR}")
    set(FFmpeg_INCLUDE_DIR "${FFmpeg_ROOT}/include")
    
    # 查找并配置库
    find_library(FFmpeg_avutil_LIBRARY NAMES avutil PATHS "${FFmpeg_ROOT}/lib" NO_DEFAULT_PATH)
    find_library(FFmpeg_avformat_LIBRARY NAMES avformat PATHS "${FFmpeg_ROOT}/lib" NO_DEFAULT_PATH)
    find_library(FFmpeg_avcodec_LIBRARY NAMES avcodec PATHS "${FFmpeg_ROOT}/lib" NO_DEFAULT_PATH)
    find_library(FFmpeg_swscale_LIBRARY NAMES swscale PATHS "${FFmpeg_ROOT}/lib" NO_DEFAULT_PATH)
    find_library(FFmpeg_swresample_LIBRARY NAMES swresample PATHS "${FFmpeg_ROOT}/lib" NO_DEFAULT_PATH)
    
    # 查找 DLLs (用于后续安装或运行时拷贝)
    file(GLOB FFmpeg_DLLS "${FFmpeg_ROOT}/bin/*.dll")
    
    if(FFmpeg_avutil_LIBRARY AND FFmpeg_avformat_LIBRARY AND FFmpeg_avcodec_LIBRARY)
        set(FFmpeg_FOUND TRUE)
        message(STATUS "✓ FFmpeg prebuilt binaries initialized via FetchContent")
        message(STATUS "  Include: ${FFmpeg_INCLUDE_DIR}")
        message(STATUS "  Libs found in: ${FFmpeg_ROOT}/lib")
    else()
        message(FATAL_ERROR "Failed to find FFmpeg libraries in downloaded archive")
    endif()
endif()

# ==================== ONNX Runtime (FetchContent Prebuilt) ====================
FetchContent_Declare(
    onnxruntime_prebuilt
    URL https://github.com/microsoft/onnxruntime/releases/download/v1.17.1/onnxruntime-win-x64-1.17.1.zip
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)

if(NOT onnxruntime_prebuilt_POPULATED)
    message(STATUS "Downloading/Extracting ONNX Runtime prebuilt binaries...")
    FetchContent_Populate(onnxruntime_prebuilt)
    
    set(ONNXRUNTIME_ROOT "${onnxruntime_prebuilt_SOURCE_DIR}")
    set(ONNXRUNTIME_INCLUDE_DIR "${ONNXRUNTIME_ROOT}/include")
    
    # 查找库文件
    find_library(ONNXRUNTIME_LIBRARY 
        NAMES onnxruntime 
        PATHS "${ONNXRUNTIME_ROOT}/lib" 
        NO_DEFAULT_PATH
    )
    
    # 查找 DLLs (用于后续运行时拷贝)
    file(GLOB ONNXRUNTIME_DLLS "${ONNXRUNTIME_ROOT}/lib/*.dll")
    
    if(ONNXRUNTIME_LIBRARY)
        set(ONNXRUNTIME_FOUND TRUE)
        message(STATUS "✓ ONNX Runtime prebuilt binaries initialized via FetchContent")
        message(STATUS "  Version: 1.17.1")
        message(STATUS "  Include: ${ONNXRUNTIME_INCLUDE_DIR}")
        message(STATUS "  Library: ${ONNXRUNTIME_LIBRARY}")
    else()
        message(FATAL_ERROR "Failed to find ONNX Runtime library in downloaded archive")
    endif()
endif()

# ==================== OpenCV (Local or FetchContent) ====================
# 1. 优先尝试通过 find_package 查找 (支持 -DOpenCV_DIR 或环境变量)
if(NOT OpenCV_FOUND)
    find_package(OpenCV CONFIG QUIET)
    if(OpenCV_FOUND)
        message(STATUS "✓ Found OpenCV via find_package: ${OpenCV_DIR}")
        set(OpenCV_LIBS ${OpenCV_LIBRARIES})
        if(NOT OpenCV_INCLUDE_DIRS)
            set(OpenCV_INCLUDE_DIRS ${OpenCV_INCLUDE_DIR})
        endif()
    endif()
endif()

# 2. 尝试在指定目录手动查找
if(NOT OpenCV_FOUND)
    set(OPENCV_CANDIDATES
        "${CMAKE_SOURCE_DIR}/deps/opencv"
        "${CMAKE_BINARY_DIR}/_deps/opencv"
        "C:/opencv"
    )
    
    foreach(CANDIDATE_DIR ${OPENCV_CANDIDATES})
        if(EXISTS "${CANDIDATE_DIR}")
            # 优先尝试 CONFIG 模式
            set(OpenCV_DIR_HINTS 
                "${CANDIDATE_DIR}"
                "${CANDIDATE_DIR}/build"
                "${CANDIDATE_DIR}/build/x64/vc16/lib"
            )
            
            foreach(HINT ${OpenCV_DIR_HINTS})
                if(EXISTS "${HINT}/OpenCVConfig.cmake")
                    find_package(OpenCV CONFIG QUIET PATHS "${HINT}" NO_DEFAULT_PATH)
                    if(OpenCV_FOUND)
                        message(STATUS "✓ Found OpenCV via CONFIG in: ${HINT}")
                        set(OpenCV_LIBS ${OpenCV_LIBRARIES})
                        break()
                    endif()
                endif()
            endforeach()

            if(OpenCV_FOUND)
                break()
            endif()

            # 如果 CONFIG 模式失败，尝试手动查找 (针对非标准结构)
            set(OpenCV_DIR "${CANDIDATE_DIR}")
            if(EXISTS "${OpenCV_DIR}/build")
                set(OpenCV_BASE "${OpenCV_DIR}/build")
            else()
                set(OpenCV_BASE "${OpenCV_DIR}")
            endif()

            set(OpenCV_INCLUDE_DIRS "${OpenCV_BASE}/include")
            file(GLOB_RECURSE OpenCV_LIB_DIR_CANDIDATES LIST_DIRECTORIES true "${OpenCV_BASE}/*/lib")
            list(APPEND OpenCV_LIB_DIR_CANDIDATES "${OpenCV_BASE}/lib")
            
            # 查找库 (增加 opencv_world 支持)
            find_library(OpenCV_world_LIBRARY 
                NAMES opencv_world4120 opencv_world480 opencv_world
                PATHS ${OpenCV_LIB_DIR_CANDIDATES}
                NO_DEFAULT_PATH
            )
            if(OpenCV_world_LIBRARY)
                set(OpenCV_FOUND TRUE)
                set(OpenCV_LIBS ${OpenCV_world_LIBRARY})
            else()
                find_library(OpenCV_core_LIBRARY 
                    NAMES opencv_core4120 opencv_core480 opencv_core
                    PATHS ${OpenCV_LIB_DIR_CANDIDATES}
                    NO_DEFAULT_PATH
                )
                find_library(OpenCV_imgproc_LIBRARY 
                    NAMES opencv_imgproc4120 opencv_imgproc480 opencv_imgproc
                    PATHS ${OpenCV_LIB_DIR_CANDIDATES}
                    NO_DEFAULT_PATH
                )
                find_library(OpenCV_imgcodecs_LIBRARY 
                    NAMES opencv_imgcodecs4120 opencv_imgcodecs480 opencv_imgcodecs
                    PATHS ${OpenCV_LIB_DIR_CANDIDATES}
                    NO_DEFAULT_PATH
                )
                if(OpenCV_core_LIBRARY)
                    set(OpenCV_FOUND TRUE)
                    set(OpenCV_LIBS ${OpenCV_core_LIBRARY} ${OpenCV_imgproc_LIBRARY} ${OpenCV_imgcodecs_LIBRARY})
                endif()
            endif()

            if(OpenCV_FOUND)
                get_filename_component(OpenCV_LIB_REAL_DIR "${OpenCV_LIBS}" DIRECTORY)
                string(REPLACE "/lib" "/bin" OpenCV_BIN_DIR "${OpenCV_LIB_REAL_DIR}")
                file(GLOB OpenCV_DLLS "${OpenCV_BIN_DIR}/*.dll")
                message(STATUS "✓ Using OpenCV from: ${CANDIDATE_DIR}")
                break()
            endif()
        endif()
    endforeach()
endif()

# 3. 如果本地没找到，再尝试 FetchContent (或者你可以注释掉这部分强制使用本地)
if(NOT OpenCV_FOUND)
    FetchContent_Declare(
        opencv_prebuilt
        URL https://github.com/thommyho/Cpp-OpenCV-Windows-PreBuilts/releases/download/v4.8.0/MSVC142_64.zip
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    )

    if(NOT opencv_prebuilt_POPULATED)
        message(STATUS "Downloading/Extracting OpenCV prebuilt binaries...")
        FetchContent_Populate(opencv_prebuilt)
        
        set(OpenCV_DIR "${opencv_prebuilt_SOURCE_DIR}")
        set(OpenCV_INCLUDE_DIRS "${OpenCV_DIR}/include")
        set(OpenCV_LIB_DIR "${OpenCV_DIR}/lib")
        set(OpenCV_BIN_DIR "${OpenCV_DIR}/bin")
        
        find_library(OpenCV_core_LIBRARY NAMES opencv_core480 PATHS "${OpenCV_LIB_DIR}" NO_DEFAULT_PATH)
        find_library(OpenCV_imgproc_LIBRARY NAMES opencv_imgproc480 PATHS "${OpenCV_LIB_DIR}" NO_DEFAULT_PATH)
        find_library(OpenCV_imgcodecs_LIBRARY NAMES opencv_imgcodecs480 PATHS "${OpenCV_LIB_DIR}" NO_DEFAULT_PATH)
        
        file(GLOB OpenCV_DLLS "${OpenCV_BIN_DIR}/*.dll")
        
        if(OpenCV_core_LIBRARY)
            set(OpenCV_FOUND TRUE)
            set(OpenCV_LIBS ${OpenCV_core_LIBRARY} ${OpenCV_imgproc_LIBRARY} ${OpenCV_imgcodecs_LIBRARY})
        endif()
    endif()
endif()

if(OpenCV_FOUND)
    message(STATUS "✓ OpenCV initialized")
else()
    message(FATAL_ERROR "OpenCV not found. Please place it in deps/opencv or ensure internet connection.")
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

# ==================== Google Test (FetchContent) ====================
include(FetchContent)

# 解决 CMP0135 警告：设置下载文件的提取时间戳
if(POLICY CMP0135)
  cmake_policy(SET CMP0135 NEW)
endif()

FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/tags/v1.15.2.zip
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

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
