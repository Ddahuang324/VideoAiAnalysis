cmake_minimum_required(VERSION 3.20)

# ==================== C++ 测试 ====================
add_executable(test_benchmark
    test_benchmark.cpp
)

target_link_libraries(test_benchmark PRIVATE ai_video_core)

# 启用测试
add_test(NAME benchmark_push_pop COMMAND test_benchmark)