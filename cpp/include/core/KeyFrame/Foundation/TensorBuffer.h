#pragma once

#include <cstddef>

namespace KeyFrame {

/**
 * @brief 张量内存缓冲池 (Arena Allocator)
 * 复用内存，减少推理过程中的动态分配开销。
 * 支持内存对齐以优化 SIMD 指令性能。
 */
class TensorBuffer {
public:
    /**
     * @param initialCapacity 初始容量（元素数量，每个元素为 float）
     * @param alignment 对齐字节数（默认 64 字节，适用于 AVX-512）
     */
    explicit TensorBuffer(size_t initialCapacity = 1024 * 1024, size_t alignment = 64);
    ~TensorBuffer();

    // 禁止拷贝
    TensorBuffer(const TensorBuffer&) = delete;
    TensorBuffer& operator=(const TensorBuffer&) = delete;

    float* allocate(size_t size);

    void reset();
    void reserve(size_t totalSize);

    size_t used() const { return offset_; }

    size_t capacity() const { return capacity_; }

private:
    void grow(size_t minSize);
    size_t alignSize(size_t size) const;

    float* data_ = nullptr;
    size_t capacity_ = 0;
    size_t offset_ = 0;
    size_t alignment_ = 64;
};

}  // namespace KeyFrame
