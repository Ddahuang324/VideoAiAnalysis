#include <iostream>

#include "benchmarkPushPop.h"

int main() {
    std::cout << "Running benchmark for ThreadSafeQueue push/pop operations..." << std::endl;
    benchmarkPushPop();
    std::cout << "Benchmark completed." << std::endl;
    return 0;
}