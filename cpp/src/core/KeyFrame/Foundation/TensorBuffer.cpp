#include "TensorBuffer.h"

#include <algorithm>
#include <cstdlib>
#include <cstring>

#include "corecrt_malloc.h"

#ifdef _WIN32
#    include <malloc.h>
#endif

namespace KeyFrame {

TensorBuffer::TensorBuffer(size_t initialCapacity, size_t alignment)
    : capacity_(initialCapacity), alignment_(alignment) {
    if (capacity_ > 0) {
        data_ = static_cast<float*>(_aligned_malloc(capacity_ * sizeof(float), alignment_));
    }
}

TensorBuffer::~TensorBuffer() {
    if (data_) {
        _aligned_free(data_);
    }
}

float* TensorBuffer::allocate(size_t size) {
    size_t alignedSize = alignSize(size);

    if (offset_ + alignedSize > capacity_) {
        grow(offset_ + alignedSize);
    }

    float* ptr = data_ + offset_;
    offset_ += alignedSize;
    return ptr;
}

void TensorBuffer::reset() {
    offset_ = 0;
}

void TensorBuffer::reserve(size_t totalSize) {  // 防抖动
    if (totalSize > capacity_) {
        grow(totalSize);
    }
}

void TensorBuffer::grow(size_t minSize) {
    size_t newCapacity = std::max(minSize, capacity_ * 2);

    float* newData = nullptr;
    newData = static_cast<float*>(_aligned_malloc(newCapacity * sizeof(float), alignment_));

    if (newData && data_ && offset_ > 0) {
        std::memcpy(newData, data_, offset_ * sizeof(float));
    }

    if (data_) {
        _aligned_free(data_);
    }

    data_ = newData;
    capacity_ = newCapacity;
}

size_t TensorBuffer::alignSize(size_t size) const {
    size_t byteSize = size * sizeof(float);
    size_t alignedByteSize = (byteSize + alignment_ - 1) & ~(alignment_ - 1);
    /*
      163: 00000000 00000000 00000000 10100011
   0xFFC0: 11111111 11111111 11111111 11000000    ~63
-------------------------------------------------
    结果:   00000000 00000000 00000000 10000000   = 128
    */
    return alignedByteSize / sizeof(float);
}

}  // namespace KeyFrame
