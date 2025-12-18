cmake_minimum_required(VERSION 3.20)

# ==================== C++ 自动化测试配置 ====================

# 宏：自动寻找指定目录下的所有 .cpp 文件并创建对应的测试可执行文件
# 参数：
#   BASE_DIR: 搜索的基准目录
#   PREFIX: 可执行文件前缀（可选）
function(register_tests_under BASE_DIR PREFIX)
    file(GLOB_RECURSE TEST_FILES "${BASE_DIR}/*.cpp")
    
    foreach(FILE_PATH ${TEST_FILES})
        # 获取相对于 UnitTests 目录的相对路径
        file(RELATIVE_PATH REL_FILE "${CMAKE_CURRENT_SOURCE_DIR}" "${FILE_PATH}")
        
        # 获取文件名（不含路径和扩展名）作为测试项目名
        get_filename_component(TEST_NAME ${FILE_PATH} NAME_WE)
        set(EXEC_NAME "${PREFIX}${TEST_NAME}")

        # 创建可执行文件
        add_executable(${EXEC_NAME} ${REL_FILE})

        # 添加头文件路径，确保 IDE 能正确识别
        target_include_directories(${EXEC_NAME} 
            PRIVATE
                ${CMAKE_SOURCE_DIR}/cpp/include
                ${CMAKE_SOURCE_DIR}/cpp/include/core/ScreenRecorder/CaptureLayer
                ${CMAKE_SOURCE_DIR}/cpp/include/core/ScreenRecorder/CaptureLayer/SpecificGrabber/Win
                ${CMAKE_SOURCE_DIR}/cpp/include/core/ScreenRecorder/ProcessLayer
                ${CMAKE_SOURCE_DIR}/cpp/include/Infra
        )

        # 链接核心库与 GTest
        target_link_libraries(${EXEC_NAME} 
            PRIVATE 
                ai_video_core
                gtest_main
        )

        # 注册到 CTest
        add_test(NAME ${EXEC_NAME} COMMAND ${EXEC_NAME})
        
        message(STATUS "✓ Registered test: ${EXEC_NAME} from ${FILE_PATH}")
    endforeach()
endfunction()

# 自动化扫描 UnitTest 目录下的所有测试文件
register_tests_under("${CMAKE_CURRENT_SOURCE_DIR}/UnitTest" "test_")

# 也可以手动注册特定的模块（如果需要特殊配置）
# add_gtest_executable(SpecialTest ...)