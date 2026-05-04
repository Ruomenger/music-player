#pragma once

#include <atomic>
#include <cassert>
#include <cstddef>
#include <vector>

namespace musicplayer {

template<typename T>
class RingBuffer {
public:
    explicit RingBuffer(size_t capacity);

    size_t write(const T* data, size_t count);
    size_t read(T* data, size_t count);
    void clear();

    size_t available() const;
    size_t freeSlots() const;
    size_t capacity() const { return buffer_.size(); }
    bool empty() const { return available() == 0; }
    bool full() const { return freeSlots() == 0; }

private:
    static constexpr size_t nextPowerOfTwo(size_t n);

    std::vector<T> buffer_;
    std::atomic<size_t> readIdx_{0};
    std::atomic<size_t> writeIdx_{0};
    size_t mask_;
};

template<typename T>
constexpr size_t RingBuffer<T>::nextPowerOfTwo(size_t n) {
    if (n == 0)
        return 1;
    size_t p = 1;
    while (p < n) {
        p <<= 1;
    }
    return p;
}

}  // namespace musicplayer
